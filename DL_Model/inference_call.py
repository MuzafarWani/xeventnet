import numpy as np
import pandas as pd
import os
import tensorflow as tf
from tensorflow.keras import backend as K
from tensorflow.keras.models import Model, load_model
from tensorflow.keras.layers import (Conv3D, ConvLSTM2D, Conv2D, MaxPooling2D, Dropout, Dense, 
                                     Flatten, TimeDistributed, GlobalMaxPooling2D, LSTM, Concatenate, Input, GlobalAveragePooling3D)
from tensorflow.keras.preprocessing.sequence import TimeseriesGenerator
from sklearn.preprocessing import LabelEncoder
from sklearn.metrics import accuracy_score

# Load model and label encoder
model_path = 'rainfall_cnn_lstm_3d_seq1_model.keras'
model = load_model(model_path)
unique_events = ["Heavy Rain", "No Event", "Snowfall", "Landslide", "Cyclone"]

label_encoder = LabelEncoder()
label_encoder.fit(unique_events)

def predict_rainfall_from_gridded_data(data_list):
    """
    Divides the domain into 10x10 spatial grids and returns 1 if any grid predicts an extreme event, else 0.
    Assumes xlat and xlong are the last two elements in data_list.
    """
    # Extract lat/lon (last two items in data_list)
    xlat = data_list[12]
    xlong = data_list[13]

    height, width = xlat.shape
    Z = data_list[6].shape[0]  # number of vertical levels

    # Iterate over 10x10 spatial grids
    for i in range(0, height - 9, 10):
        for j in range(0, width - 9, 10):
            # Extract 2D variables (shape: (10, 10))
            rainc   = data_list[0][i:i+10, j:j+10]
            rainnc  = data_list[1][i:i+10, j:j+10]
            olr     = data_list[2][i:i+10, j:j+10]
            snow    = data_list[3][i:i+10, j:j+10]
            snowh   = data_list[4][i:i+10, j:j+10]
            psfc    = data_list[5][i:i+10, j:j+10]

            features_2d = np.stack([rainc, rainnc, olr, snow, snowh, psfc], axis=-1)  # (10, 10, 6)
            features_2d = np.expand_dims(features_2d, axis=0)  # (1, 10, 10, 6)

            # Extract 3D variables (shape: (Z, 10, 10))
            p       = data_list[6][:, i:i+10, j:j+10]
            u       = data_list[7][:, i:i+10, j:j+10]
            v       = data_list[8][:, i:i+10, j:j+10]
            t       = data_list[9][:, i:i+10, j:j+10]
            pb      = data_list[10][:, i:i+10, j:j+10]
            qvapor  = data_list[11][:, i:i+10, j:j+10]

            features_3d = np.stack([p, u, v, t, pb, qvapor], axis=-1)  # (Z, 10, 10, 6)
            features_3d = np.expand_dims(features_3d, axis=0)         # (1, Z, 10, 10, 6)

            # Predict
            prediction = model.predict([features_2d, features_3d], verbose=0)
            predicted_label = np.argmax(prediction[0])
            decoded_prediction = label_encoder.inverse_transform([predicted_label])[0]

            if decoded_prediction != "No Event":
                print(f"Predicted extreme event: {decoded_prediction} at grid ({i}:{i+10}, {j}:{j+10})")
                return 1  # Extreme event detected

    return 0  # No extreme events detected

