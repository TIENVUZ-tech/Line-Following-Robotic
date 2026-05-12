using System;
using System.Text;
using System.Threading.Tasks;
using Windows.Devices.Bluetooth;
using Windows.Devices.Bluetooth.GenericAttributeProfile;
using System.Runtime.InteropServices.WindowsRuntime;
using Windows.Storage.Streams;

namespace BleUartReader
{
    class BleUartClient
    {
        public event EventHandler<string>? DataReceived;
        private static readonly ulong BLE_MAC_ADDRESS = 0xBE2F786028E6;
        private static readonly Guid UART_SERVICE_UUID = Guid.Parse("0000ffe0-0000-1000-8000-00805f9b34fb");
        private static readonly Guid UART_RX_CHARACTERISTIC_UUID = Guid.Parse("0000ffe1-0000-1000-8000-00805f9b34fb");

        private GattCharacteristic? rxCharacteristic;

        public async Task RunAsync()
        {
            Console.WriteLine("Scanning for LINE ROBOT...");

            try
            {
                // 1. Connect to the device using its MAC address
                using BluetoothLEDevice bleDevice = await BluetoothLEDevice.FromBluetoothAddressAsync(BLE_MAC_ADDRESS);

                if (bleDevice == null)
                {
                    Console.WriteLine("Failed to find device. Ensure it is powered on and paired to Windows.");
                    return;
                }

                Console.WriteLine($"Connected to: {bleDevice.Name}");

                // 2. Look for the UART Service (FFE0)
                GattDeviceServicesResult serviceResult = await bleDevice.GetGattServicesForUuidAsync(UART_SERVICE_UUID, BluetoothCacheMode.Uncached);
                if (serviceResult.Status != GattCommunicationStatus.Success || serviceResult.Services.Count == 0)
                {
                    Console.WriteLine("Could not find the UART Service (FFE0).");
                    return;
                }
                GattDeviceService uartService = serviceResult.Services[0];

                // Open the service session before querying characteristics
                await uartService.OpenAsync(GattSharingMode.SharedReadAndWrite);

                // 3. Look for the RX Characteristic (FFE1)
                GattCharacteristicsResult charResult = await uartService.GetCharacteristicsForUuidAsync(UART_RX_CHARACTERISTIC_UUID, BluetoothCacheMode.Uncached);
                if (charResult.Status != GattCommunicationStatus.Success || charResult.Characteristics.Count == 0)
                {
                    Console.WriteLine("Could not find the RX Characteristic (FFE1).");
                    return;
                }
                rxCharacteristic = charResult.Characteristics[0];

                // 4. Tell the module to start sending Notifications
                GattCommunicationStatus status = await rxCharacteristic.WriteClientCharacteristicConfigurationDescriptorAsync(
                    GattClientCharacteristicConfigurationDescriptorValue.Notify);

                if (status == GattCommunicationStatus.Success)
                {
                    // 5. Attach the event handler that runs when data arrives
                    rxCharacteristic.ValueChanged += Characteristic_ValueChanged;
                    Console.WriteLine("Successfully connected! Listening for STM32 data...");
                    Console.WriteLine("==================================================\n");
                }
                else
                {
                    Console.WriteLine("Failed to subscribe to notifications.");
                }

                // Listen for key presses to send commands
                Console.WriteLine("Press [1] to send '1', [0] to send '0', [ESC] to exit.\n");
                while (true)
                {
                    ConsoleKey key = Console.ReadKey(intercept: true).Key;

                    if (key == ConsoleKey.D1)
                        await SendOneAsync();
                    else if (key == ConsoleKey.D0)
                        await SendZeroAsync();
                    else if (key == ConsoleKey.Escape)
                        break;
                }
            }
            catch (Exception ex)
            {
                Console.WriteLine($"An error occurred: {ex.Message}");
            }
        }

        // Sends "1" to the robot
        public async Task SendOneAsync()
        {
            await SendAsync("1\n");
        }

        // Sends "0" to the robot
        public async Task SendZeroAsync()
        {
            await SendAsync("0\n");
        }

        public async Task SendDataAsync(string data)
        {
            await SendAsync(data);
        }

        private async Task SendAsync(string value)
        {
            if (rxCharacteristic == null) return;

            var props = rxCharacteristic.CharacteristicProperties;
            GattWriteOption writeOption;

            if (props.HasFlag(GattCharacteristicProperties.Write))
                writeOption = GattWriteOption.WriteWithResponse;
            else if (props.HasFlag(GattCharacteristicProperties.WriteWithoutResponse))
                writeOption = GattWriteOption.WriteWithoutResponse;
            else
            {
                Console.WriteLine("Characteristic does not support writing.");
                return;
            }

            byte[] bytes = Encoding.UTF8.GetBytes(value);
            IBuffer buffer = bytes.AsBuffer();
            GattCommunicationStatus status = await rxCharacteristic.WriteValueAsync(buffer, writeOption);

            if (status == GattCommunicationStatus.Success)
                Console.WriteLine($"Sent: {value}");
            else
                Console.WriteLine($"Failed to send: {value}");
        }

        // This function triggers automatically every time the STM32 sends a Bluetooth packet
        private void Characteristic_ValueChanged(GattCharacteristic sender, GattValueChangedEventArgs args)
        {
            byte[] data = args.CharacteristicValue.ToArray();
            string receivedString = Encoding.UTF8.GetString(data);
            Console.Write(receivedString);
            DataReceived?.Invoke(this, receivedString);
        }
    }
}
