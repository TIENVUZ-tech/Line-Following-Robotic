using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Text.RegularExpressions;
using System.Threading.Tasks;
using System.Windows.Forms;
using System.IO.Ports;

namespace LineFollowingRobot_PC
{
    public partial class Form1 : Form
    {
        private SerialPort serialPort;
        private string receivedData = "";
        private CarState currentState = CarState.UNKNOWN;
        private Dictionary<string, CarState> stateMap;
        private Queue<DateTime> stateChangeTimestamps;
        private int stateChangeCount = 0;

        public enum CarState
        {
            STOP,
            FOLLOW_LINE,
            TURN_LEFT,
            TURN_RIGHT,
            SEARCH_MAZE,
            UNKNOWN
        }

        public Form1()
        {
            InitializeComponent();
            InitializeSerialPort();
            InitializeStateMap();
            stateChangeTimestamps = new Queue<DateTime>();
        }

        private void InitializeSerialPort()
        {
            serialPort = new SerialPort();
            serialPort.BaudRate = 115200;
            serialPort.DataBits = 8;
            serialPort.StopBits = StopBits.One;
            serialPort.Parity = Parity.None;
            serialPort.DataReceived += SerialPort_DataReceived;
        }

        private void InitializeStateMap()
        {
            stateMap = new Dictionary<string, CarState>()
            {
                { "STOP", CarState.STOP },
                { "FOLLOW_LINE", CarState.FOLLOW_LINE },
                { "TURN_LEFT", CarState.TURN_LEFT },
                { "TURN_RIGHT", CarState.TURN_RIGHT },
                { "SEARCH_MAZE", CarState.SEARCH_MAZE }
            };
        }

        private void SerialPort_DataReceived(object sender, SerialDataReceivedEventArgs e)
        {
            try
            {
                string data = serialPort.ReadExisting();
                receivedData += data;

                // Parse messages in format: $STATE#
                Regex regex = new Regex(@"\$([A-Z_]+)#");
                MatchCollection matches = regex.Matches(receivedData);

                if (matches.Count > 0)
                {
                    // Get the last complete message
                    Match lastMatch = matches[matches.Count - 1];
                    string stateName = lastMatch.Groups[1].Value;

                    if (stateMap.ContainsKey(stateName))
                    {
                        CarState newState = stateMap[stateName];
                        if (newState != currentState)
                        {
                            currentState = newState;
                            stateChangeCount++;
                            stateChangeTimestamps.Enqueue(DateTime.Now);

                            // Keep only last 60 state changes (for 1-minute window)
                            if (stateChangeTimestamps.Count > 60)
                            {
                                stateChangeTimestamps.Dequeue();
                            }
                        }
                    }

                    // Clear received data up to the end of the last message
                    receivedData = receivedData.Substring(receivedData.LastIndexOf('#') + 1);
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show("Error reading from serial port: " + ex.Message);
            }
        }

        private void UpdateUI()
        {
            if (InvokeRequired)
            {
                Invoke(new Action(UpdateUI));
                return;
            }

            // Update status label
            string statusText = GetStatusText(currentState);
            labelStatus.Text = "Trạng thái: " + statusText;
            labelStatus.ForeColor = GetStatusColor(currentState);

            // Update state change counter
            labelStateCount.Text = "Thay đổi trạng thái: " + stateChangeCount.ToString();
        }

        private string GetStatusText(CarState state)
        {
            return state switch
            {
                CarState.STOP => "Dừng lại",
                CarState.FOLLOW_LINE => "Đang đi thẳng theo line",
                CarState.TURN_LEFT => "Đang rẽ trái",
                CarState.TURN_RIGHT => "Đang rẽ phải",
                CarState.SEARCH_MAZE => "Đang tìm đường trong mê cung",
                _ => "Không xác định"
            };
        }

        private Color GetStatusColor(CarState state)
        {
            return state switch
            {
                CarState.STOP => Color.Red,
                CarState.FOLLOW_LINE => Color.Green,
                CarState.TURN_LEFT => Color.Orange,
                CarState.TURN_RIGHT => Color.Orange,
                CarState.SEARCH_MAZE => Color.Blue,
                _ => Color.Black
            };
        }

        private void buttonConnect_Click(object sender, EventArgs e)
        {
            if (serialPort.IsOpen)
            {
                serialPort.Close();
                buttonConnect.Text = "Kết nối";
                buttonStart.Enabled = false;
                buttonStop.Enabled = false;
                labelStatus.Text = "Trạng thái: Chưa kết nối";
                labelStatus.ForeColor = Color.Black;
            }
            else
            {
                try
                {
                    string portName = comboBoxPort.SelectedItem?.ToString();
                    if (string.IsNullOrEmpty(portName))
                    {
                        MessageBox.Show("Vui lòng chọn cổng COM");
                        return;
                    }

                    serialPort.PortName = portName;
                    serialPort.Open();
                    buttonConnect.Text = "Ngắt kết nối";
                    buttonStart.Enabled = true;
                    buttonStop.Enabled = true;
                    labelStatus.Text = "Trạng thái: Đã kết nối";
                    labelStatus.ForeColor = Color.DarkGreen;
                    comboBoxPort.Enabled = false;
                }
                catch (Exception ex)
                {
                    MessageBox.Show("Lỗi kết nối: " + ex.Message);
                }
            }
        }

        private void buttonStart_Click(object sender, EventArgs e)
        {
            if (serialPort.IsOpen)
            {
                try
                {
                    serialPort.Write("G");
                    labelCommand.Text = "Lệnh gửi: START (G)";
                }
                catch (Exception ex)
                {
                    MessageBox.Show("Lỗi gửi lệnh: " + ex.Message);
                }
            }
        }

        private void buttonStop_Click(object sender, EventArgs e)
        {
            if (serialPort.IsOpen)
            {
                try
                {
                    serialPort.Write("S");
                    labelCommand.Text = "Lệnh gửi: STOP (S)";
                }
                catch (Exception ex)
                {
                    MessageBox.Show("Lỗi gửi lệnh: " + ex.Message);
                }
            }
        }

        private void Form1_Load(object sender, EventArgs e)
        {
            // Populate COM ports
            string[] ports = SerialPort.GetPortNames();
            comboBoxPort.Items.AddRange(ports);
            if (ports.Length > 0)
            {
                comboBoxPort.SelectedIndex = 0;
            }

            // Timer for UI updates
            Timer updateTimer = new Timer();
            updateTimer.Interval = 200; // Update every 200ms
            updateTimer.Tick += (s, e) => UpdateUI();
            updateTimer.Start();

            buttonStart.Enabled = false;
            buttonStop.Enabled = false;
            labelStatus.Text = "Trạng thái: Chưa kết nối";
        }

        private void Form1_FormClosing(object sender, FormClosingEventArgs e)
        {
            if (serialPort.IsOpen)
            {
                serialPort.Close();
            }
            serialPort.Dispose();
        }

        private void InitializeComponent()
        {
            this.comboBoxPort = new System.Windows.Forms.ComboBox();
            this.buttonConnect = new System.Windows.Forms.Button();
            this.buttonStart = new System.Windows.Forms.Button();
            this.buttonStop = new System.Windows.Forms.Button();
            this.labelStatus = new System.Windows.Forms.Label();
            this.labelCommand = new System.Windows.Forms.Label();
            this.labelStateCount = new System.Windows.Forms.Label();
            this.labelPortSelect = new System.Windows.Forms.Label();

            this.SuspendLayout();

            // comboBoxPort
            this.comboBoxPort.Location = new System.Drawing.Point(12, 30);
            this.comboBoxPort.Name = "comboBoxPort";
            this.comboBoxPort.Size = new System.Drawing.Size(150, 23);
            this.comboBoxPort.TabIndex = 0;

            // buttonConnect
            this.buttonConnect.Location = new System.Drawing.Point(168, 30);
            this.buttonConnect.Name = "buttonConnect";
            this.buttonConnect.Size = new System.Drawing.Size(100, 23);
            this.buttonConnect.TabIndex = 1;
            this.buttonConnect.Text = "Kết nối";
            this.buttonConnect.UseVisualStyleBackColor = true;
            this.buttonConnect.Click += new System.EventHandler(this.buttonConnect_Click);

            // labelPortSelect
            this.labelPortSelect.AutoSize = true;
            this.labelPortSelect.Location = new System.Drawing.Point(12, 12);
            this.labelPortSelect.Name = "labelPortSelect";
            this.labelPortSelect.Size = new System.Drawing.Size(75, 15);
            this.labelPortSelect.TabIndex = 2;
            this.labelPortSelect.Text = "Chọn cổng COM:";

            // labelStatus
            this.labelStatus.AutoSize = true;
            this.labelStatus.Font = new System.Drawing.Font("Segoe UI", 14F, System.Drawing.FontStyle.Bold);
            this.labelStatus.Location = new System.Drawing.Point(12, 80);
            this.labelStatus.Name = "labelStatus";
            this.labelStatus.Size = new System.Drawing.Size(200, 32);
            this.labelStatus.TabIndex = 3;
            this.labelStatus.Text = "Trạng thái: Chưa kết nối";

            // buttonStart
            this.buttonStart.BackColor = System.Drawing.Color.LimeGreen;
            this.buttonStart.Font = new System.Drawing.Font("Segoe UI", 12F, System.Drawing.FontStyle.Bold);
            this.buttonStart.Location = new System.Drawing.Point(12, 150);
            this.buttonStart.Name = "buttonStart";
            this.buttonStart.Size = new System.Drawing.Size(120, 50);
            this.buttonStart.TabIndex = 4;
            this.buttonStart.Text = "START (G)";
            this.buttonStart.UseVisualStyleBackColor = false;
            this.buttonStart.Click += new System.EventHandler(this.buttonStart_Click);

            // buttonStop
            this.buttonStop.BackColor = System.Drawing.Color.Red;
            this.buttonStop.Font = new System.Drawing.Font("Segoe UI", 12F, System.Drawing.FontStyle.Bold);
            this.buttonStop.ForeColor = System.Drawing.Color.White;
            this.buttonStop.Location = new System.Drawing.Point(148, 150);
            this.buttonStop.Name = "buttonStop";
            this.buttonStop.Size = new System.Drawing.Size(120, 50);
            this.buttonStop.TabIndex = 5;
            this.buttonStop.Text = "STOP (S)";
            this.buttonStop.UseVisualStyleBackColor = false;
            this.buttonStop.Click += new System.EventHandler(this.buttonStop_Click);

            // labelCommand
            this.labelCommand.AutoSize = true;
            this.labelCommand.Location = new System.Drawing.Point(12, 220);
            this.labelCommand.Name = "labelCommand";
            this.labelCommand.Size = new System.Drawing.Size(100, 15);
            this.labelCommand.TabIndex = 6;
            this.labelCommand.Text = "Lệnh đã gửi:";

            // labelStateCount
            this.labelStateCount.AutoSize = true;
            this.labelStateCount.Location = new System.Drawing.Point(12, 250);
            this.labelStateCount.Name = "labelStateCount";
            this.labelStateCount.Size = new System.Drawing.Size(100, 15);
            this.labelStateCount.TabIndex = 7;
            this.labelStateCount.Text = "Thay đổi trạng thái: 0";

            // Form1
            this.AutoScaleDimensions = new System.Drawing.SizeF(7F, 15F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(280, 280);
            this.Controls.Add(this.labelStateCount);
            this.Controls.Add(this.labelCommand);
            this.Controls.Add(this.buttonStop);
            this.Controls.Add(this.buttonStart);
            this.Controls.Add(this.labelStatus);
            this.Controls.Add(this.labelPortSelect);
            this.Controls.Add(this.buttonConnect);
            this.Controls.Add(this.comboBoxPort);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedSingle;
            this.MaximizeBox = false;
            this.MinimizeBox = true;
            this.Name = "Form1";
            this.Text = "Line Following Robot PC Telemetry";
            this.FormClosing += new System.Windows.Forms.FormClosingEventHandler(this.Form1_FormClosing);
            this.Load += new System.EventHandler(this.Form1_Load);
            this.ResumeLayout(false);
            this.PerformLayout();
        }

        private System.Windows.Forms.ComboBox comboBoxPort;
        private System.Windows.Forms.Button buttonConnect;
        private System.Windows.Forms.Button buttonStart;
        private System.Windows.Forms.Button buttonStop;
        private System.Windows.Forms.Label labelStatus;
        private System.Windows.Forms.Label labelCommand;
        private System.Windows.Forms.Label labelStateCount;
        private System.Windows.Forms.Label labelPortSelect;
    }
}
