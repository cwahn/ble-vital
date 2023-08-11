#include <iostream>
#include <thread>
#include <chrono>

#include "efp.hpp"
#include "ble-vital-client.hpp"

using namespace efp;

std::mutex ble_frame_mutex_{};
BleFrame ble_frame{};
float ble_read_duration_ms{};
bool is_new_frame = false;
bool is_ble_connected_ = false;

SimpleBLE::Peripheral found_peripheral_;

// Main code
int main(int, char **)
{
    const auto callback = [](const BleFrame *p_ble_frame, const float &read_duration_ms)
    {
        ArrayView<float, 125> fft_view{(float *)&(p_ble_frame->fft)};
        for_each_with_index([](int i, auto x)
                            { ble_frame.fft[i] = (x >= 0 ? x : -x) / 128.; },
                            fft_view);

        printf("spo2: %f, heart_rate: %f, temperature: %f, read_duration: %f\n",
               p_ble_frame->spo2,
               p_ble_frame->heart_rate,
               p_ble_frame->temperature,
               read_duration_ms);
        // ble_frame.spo2 = p_ble_frame->spo2;
        // ble_frame.heart_rate = p_ble_frame->heart_rate;
        // ble_frame.temperature = p_ble_frame->temperature;
        // flost ble_read_duration_ms = read_duration_ms.;
        // is_new_frame = true;
    };

    BleVitalClient client{callback};

    while (true)
    {
        std::cout << "Main thread alive" << std::endl;
        std::this_thread::sleep_for(std ::chrono::milliseconds(5000));
    }

    return 0;
}
