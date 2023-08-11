#ifndef BLE_VITAL_CLIENT_HPP_
#define BLE_VITAL_CLIENT_HPP_

#include <chrono>
#include <iostream>
#include <thread>
#include <vector>
#include <algorithm>

#include "efp.hpp"

#include "simpleble/SimpleBLE.h"
#include "simpleble/Logging.h"

#include "../src/utils.hpp"

struct GapCallback
{
    std::function<void(SimpleBLE::Adapter &)> on_scan_start;
    std::function<void(SimpleBLE::Adapter &)> on_scan_stop;
    std::function<void(SimpleBLE::Adapter &, SimpleBLE::Peripheral)> on_scan_found;
    std::function<void(SimpleBLE::Adapter &, SimpleBLE::Peripheral)> on_scan_update;
};

struct ClientConfig
{
    std::function<void(SimpleBLE::Adapter &)> setup;
    GapCallback gap_callback;
};

class Client
{
public:
    Client(ClientConfig config)
        : config_(config)
    {
        adapter_ = Utils::getAdapter().value();

        adapter_.set_callback_on_scan_start(
            [&]()
            { config_.gap_callback.on_scan_start(adapter_); });
        adapter_.set_callback_on_scan_stop(
            [&]()
            { config_.gap_callback.on_scan_stop(adapter_); });
        adapter_.set_callback_on_scan_updated(
            [&](SimpleBLE::Peripheral p)
            { config_.gap_callback.on_scan_update(adapter_, p); });
        adapter_.set_callback_on_scan_found(
            [&](SimpleBLE::Peripheral p)
            { config_.gap_callback.on_scan_found(adapter_, p); });

        config_.setup(adapter_);

        adapter_.scan_start();
    }
    ~Client() {}

private:
    const ClientConfig config_;
    SimpleBLE::Adapter adapter_;
};

// target_identifier and BleFrame are hard coded on HW.

constexpr const char *target_identifier = "Ars Vivendi BLE";
struct BleFrame
{
    float spo2;
    float heart_rate;
    float temperature;
    float fft[125];
};

// May costumize if more control on connection policy or callback parameter.

class BleVitalClient
{
public:
    template <typename F>
    BleVitalClient(const F &callback)
        : client_{{[&](SimpleBLE::Adapter &adp)
                   {
                       std::cout << "Setting up GATT client" << std::endl;
                   },
                   {[&](SimpleBLE::Adapter &adp)
                    {
                        std::cout << "Starting scan" << std::endl;
                    },
                    [&](SimpleBLE::Adapter &adp)
                    {
                        std::cout << "Scan stopped" << std::endl;
                    },
                    [&](SimpleBLE::Adapter &adp, SimpleBLE::Peripheral p)
                    {
                        const auto id = p.identifier();
                        std::cout << "Found device: " << id << " [" << p.address() << "] " << p.rssi() << " dBm" << std::endl;

                        if (id == target_identifier)
                        {
                            std::cout << "Target device found" << std::endl;
                            // found_peripheral = p;
                            adp.scan_stop();

                            auto read_task = [&](SimpleBLE::Peripheral p)
                            {
                                std::cout << "Connecting to the device" << std::endl;
                                while (!p.is_connected())
                                {
                                    try
                                    {
                                        p.connect();
                                    }
                                    catch (...)
                                    {
                                        std::cout << "Connecting failed with exception. Retrying" << std::endl;
                                    }
                                }

                                std::cout << "Connected. MTU: " << p.mtu() << std::endl;
                                is_ble_connected = true;

                                auto service_uuid = p.services()[0].uuid();
                                auto characteristic_uuid = p.services()[0].characteristics()[0].uuid();

                                while (p.is_connected())
                                {
                                    try
                                    {
                                        const auto read_start = std::chrono::high_resolution_clock::now();
                                        SimpleBLE::ByteArray rx_data = p.read(service_uuid, characteristic_uuid);
                                        const auto read_end = std::chrono::high_resolution_clock::now();
                                        const auto read_duration = std::chrono::duration_cast<std::chrono::milliseconds>(read_end - read_start);
                                        BleFrame *p_ble_frame = (BleFrame *)rx_data.c_str();

                                        {
                                            callback(p_ble_frame, read_duration.count());
                                        }

                                        // std::cout << "Received " << rx_data.size() << " bytes ";
                                        // std::cout << "in " << std::dec << read_duration.count() << " ms" << std::endl;
                                    }
                                    catch (SimpleBLE::Exception::OperationFailed &ex)
                                    {
                                        if (!p.is_connected())
                                        {
                                            std::cout << "Peripheral disconnected with exception" << std::endl;
                                            is_ble_connected = false;
                                            adp.scan_start();
                                        }
                                    }
                                }

                                std::cout << "Peripheral disconnected" << std::endl;
                                is_ble_connected = false;
                                adp.scan_start();
                            };

                            std::thread read_thread(read_task, p);
                            read_thread.detach();
                        }
                    },
                    [&](SimpleBLE::Adapter &adp, SimpleBLE::Peripheral p) {

                    }}}}
    {
        // const auto setup_task =

        // const GapCallback gap_callback = ;

        // ClientConfig config{setup_task, gap_callback};
        // client_ = Client{config};
    }

private:
    // std::mutex ble_frame_mutex_{};
    // BleFrame ble_frame{};
    // float ble_read_duration_ms{};
    // bool is_new_frame = false;
    bool is_ble_connected = false;
    SimpleBLE::Peripheral found_peripheral;
    Client client_;
};

#endif