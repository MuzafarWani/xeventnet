import numpy as np
import pandas as pd
import netCDF4 as nc
import os
import tensorflow as tf
from tensorflow.keras import backend as K
from tensorflow.keras.models import Model, load_model
from tensorflow.keras.layers import (Conv3D, Conv2D, MaxPooling2D, Dropout, Dense, 
                                     Flatten, TimeDistributed, GlobalMaxPooling2D, 
                                     GlobalAveragePooling3D, Concatenate, Input)
from sklearn.preprocessing import LabelEncoder

# Set up multi-worker strategy on single node
strategy = tf.distribute.MirroredStrategy()
#strategy = tf.distribute.MultiWorkerMirroredStrategy()


# ------------------ Data Loading Functions ------------------

def load_wrf_data(wrf_file):
    dataset = nc.Dataset(wrf_file)
    return (
        dataset.variables['RAINC'][:], dataset.variables['RAINNC'][:], dataset.variables['OLR'][:],
        dataset.variables['SNOW'][:], dataset.variables['SNOWH'][:], dataset.variables['PSFC'][:],
        dataset.variables['P'][:], dataset.variables['U'][:], dataset.variables['V'][:],
        dataset.variables['T'][:], dataset.variables['PB'][:], dataset.variables['QVAPOR'][:],
        dataset.variables['XLAT'][:], dataset.variables['XLONG'][:]
    )

def load_location_data(location_file, unique_events):
    data = pd.read_csv(location_file, header=None, names=['timestep', 'xlat', 'xlong', 'rainfall_type'])
    label_encoder = LabelEncoder()
    label_encoder.fit(unique_events)
    data['rainfall_type_encoded'] = label_encoder.transform(data['rainfall_type'])
    return data, label_encoder

def find_nearest_grid_point(lat, lon, lats, lons):
    lat_diff = np.abs(lats - lat)
    lon_diff = np.abs(lons - lon)
    min_diff = np.sqrt(lat_diff**2 + lon_diff**2)
    return np.unravel_index(np.argmin(min_diff, axis=None), min_diff.shape)

def prepare_dataset(wrf_data, location_data, lats, lons):
    features_2d, features_3d, labels = [], [], []

    rainc, rainnc, olr, snow, snowh, psfc, p, u, v, t, pb, qvapor = wrf_data

    for _, row in location_data.iterrows():
        timestep = int(row['timestep'])
        xlat, xlong = row['xlat'], row['xlong']
        lat_idx, lon_idx = find_nearest_grid_point(xlat, xlong, lats[timestep], lons[timestep])

        rainc_grid = rainc[timestep, lat_idx-5:lat_idx+5, lon_idx-5:lon_idx+5]
        rainnc_grid = rainnc[timestep, lat_idx-5:lat_idx+5, lon_idx-5:lon_idx+5]
        olr_grid = olr[timestep, lat_idx-5:lat_idx+5, lon_idx-5:lon_idx+5]
        snow_grid = snow[timestep, lat_idx-5:lat_idx+5, lon_idx-5:lon_idx+5]
        snowh_grid = snowh[timestep, lat_idx-5:lat_idx+5, lon_idx-5:lon_idx+5]
        psfc_grid = psfc[timestep, lat_idx-5:lat_idx+5, lon_idx-5:lon_idx+5]

        feature_2d_grid = np.stack([rainc_grid, rainnc_grid, olr_grid, snow_grid, snowh_grid, psfc_grid], axis=-1)
        features_2d.append(feature_2d_grid)

        p_grid = p[timestep, :, lat_idx-5:lat_idx+5, lon_idx-5:lon_idx+5]
        u_grid = u[timestep, :, lat_idx-5:lat_idx+5, lon_idx-5:lon_idx+5]
        v_grid = v[timestep, :, lat_idx-5:lat_idx+5, lon_idx-5:lon_idx+5]
        t_grid = t[timestep, :, lat_idx-5:lat_idx+5, lon_idx-5:lon_idx+5]
        pb_grid = pb[timestep, :, lat_idx-5:lat_idx+5, lon_idx-5:lon_idx+5]
        qvapor_grid = qvapor[timestep, :, lat_idx-5:lat_idx+5, lon_idx-5:lon_idx+5]

        feature_3d_grid = np.stack([p_grid, u_grid, v_grid, t_grid, pb_grid, qvapor_grid], axis=-1)
        features_3d.append(feature_3d_grid)

        labels.append(row['rainfall_type_encoded'])

    return np.array(features_2d), np.array(features_3d), np.array(labels)


# ------------------ Model Building ------------------

def build_model(input_shape_2d, input_shape_3d):
    input_2d = Input(shape=input_shape_2d)
    x2d = Conv2D(32, (3, 3), activation='relu', padding='same')(input_2d)
    x2d = Conv2D(64, (3, 3), activation='relu', padding='same')(x2d)
    x2d = MaxPooling2D((2, 2))(x2d)
    x2d = Conv2D(128, (3, 3), activation='relu', padding='same')(x2d)
    x2d = GlobalMaxPooling2D()(x2d)
    x2d = Dense(128, activation='relu')(x2d)

    input_3d = Input(shape=input_shape_3d)
    x3d = Conv3D(32, (3, 3, 3), activation='relu', padding='same')(input_3d)
    x3d = Conv3D(64, (3, 3, 3), activation='relu', padding='same')(x3d)
    x3d = MaxPooling3D((2, 2, 2))(x3d)
    x3d = Conv3D(128, (3, 3, 3), activation='relu', padding='same')(x3d)
    x3d = GlobalAveragePooling3D()(x3d)
    x3d = Dense(128, activation='relu')(x3d)

    combined = Concatenate()([x2d, x3d])
    combined = Dense(128, activation='relu')(combined)
    combined = Dropout(0.5)(combined)
    output = Dense(5, activation='softmax')(combined)

    model = Model(inputs=[input_2d, input_3d], outputs=output)
    model.compile(optimizer='adam', loss='sparse_categorical_crossentropy', metrics=['accuracy'])
    return model

# ------------------ Training Function ------------------

def train_distributed_model(wrf_file, location_file, unique_events, model_path='rainfall_model.keras'):
    rainc, rainnc, olr, snow, snowh, psfc, p, u, v, t, pb, qvapor, lats, lons = load_wrf_data(wrf_file)
    location_data, label_encoder = load_location_data(location_file, unique_events)

    features_2d, features_3d, labels = prepare_dataset((rainc, rainnc, olr, snow, snowh, psfc, p, u, v, t, pb, qvapor), location_data, lats, lons)
    num_samples = len(features_2d)

    features_2d = features_2d.reshape(num_samples, 10, 10, 6)
    features_3d = features_3d.reshape(num_samples, p.shape[1], 10, 10, 6)
    labels = labels.reshape(num_samples)

    input_shape_2d = (10, 10, 6)
    input_shape_3d = (p.shape[1], 10, 10, 6)

    with strategy.scope():
        model = build_model(input_shape_2d, input_shape_3d)

    print("Training in distributed mode...")
    model.fit([features_2d, features_3d], labels, epochs=10, batch_size=32)
    model.save(model_path)
    print(f"Model saved to {model_path}")
    return model, label_encoder

# ------------------ Usage -----------------
    



# Example usage

wrf_file = 'Cyclone_Gulaab_2021-09-24_5_days'
location_file = 'Cyclone_Gulaab_2021-09-24_5_days_label.txt'
unique_events = ["Heavy Rain", "No Event", "Snowfall", "Landslide", "Cyclone"]
print("Available devices:")
for device in tf.config.list_physical_devices():
    print(device)
train_distributed_model(wrf_file, location_file, unique_events)

