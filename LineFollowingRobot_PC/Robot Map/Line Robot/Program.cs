using System;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace BleUartReader
{
    class Program
    {
        [STAThread]
        static void Main()
        {
            Application.EnableVisualStyles();
            Application.SetCompatibleTextRenderingDefault(false);

            var client = new BleUartClient();
            var form = new RobotMapForm(client);

            // BLE runs on a background thread; the map form runs on the UI thread
            _ = Task.Run(() => client.RunAsync());

            Application.Run(form);
        }
    }
}
