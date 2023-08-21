# BLE Vital Device Client
Ars Vivendi BLE vital client

## Feature
- Search for BLE device advertising with the name "Ars Vivendi BLE"
- Read data continuously from the device in a busy loop.
- On disconnect or connection failure, try to search the device and recover the connection.
  - The device will do the same. On disconnect, automatically restart advertising.

## Data Structure
Each BLE read will contain following information.
```cpp
struct BleFrame
{
    int spo2; // Should deviced by 1000 to get fixed point float.
    int heart_rate; // Should deviced by 1000 to get fixed point float.
    int temperature; // Should deviced by 1000 to get fixed point float.
    int fft[125]; // Should deviced by 1000 to get fixed point float.
};
```

## Signal Processing Detail
### Sound
The device will sample analog sound signals with internal ADC at 20480 Hz.

The raw ADC data will be decimated to 1024 Hz. (i.e. Filtered with first order low pass filter of cutoff frequency 512 Hz, discretized with bilinear transformation with sampling rate 20480 Hz. Then, downsampled to 1/20 to result in 1024 Hz.) The decimated signal will be stored in FIFO queue of the capacity 254 samples.

Queued data will be poped in every 62500 us, which means expecting about 64 new samples. If more than 64 samples are queued, data will get poped, otherwise, wait for next period.

Poped 64 samples will be pushed to a sliding window of size 128. The data in the sliding window is preprocessed by removing DC and detrending (1st order) in sequence. The preprocessed data went through real FFT to result in 128 frequency domain signal. The first 125 signals will be queued in the next BLE data. 

### SPO2
The Max30102 SPO2 sensor will do sampling at 200 Hz and decimated by 32 to result in 6.25 Hz. Each sample will contain red LEDs, IR LEDs, temperature. 

The sample will be read every 160 ms. LED data will be pushed to a sliding window of size 256. Data in the window will preprocessed by removing DC and detrending in sequence. Detrended data is used to evaluate, correlation, heart rate, and SPO2.

$$ Correlation = Pearson Correlation (Red, IR) $$

$$ heart rate = argmax(FFT (IR)) * 6.25 Hz / (256 * 2) * 60 \ BPS $$

$$ SPO2 = 104 * \frac{Red_{AC}/ Red_{Dc}}{Ir_{AC}/ Ir_{Dc}} - 17 $$

The heart rate and SPO2 result will be disregarded in case the correlation value is smaller than 0.7 assuming there is no target on the sensor and the data will be zero. The latest data from max30102 will be included in BLE frame. Therefore, any subsequent reads with in 160 ms may get the same result. 

## Requirements 
- Tested for C++20 or later
- Need BLE supporting HW. 