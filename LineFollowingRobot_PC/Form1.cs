using System;
using System.Collections.Generic;
using System.Drawing;
using System.Drawing.Drawing2D;
using System.IO.Ports;
using System.Text.RegularExpressions;
using System.Windows.Forms;

namespace LineFollowingRobot_PC
{
    public partial class Form1 : Form
    {
        // ─── Serial Port ──────────────────────────────────────────────
        private SerialPort serialPort;
        private string     receivedBuffer = "";

        // ─── Trạng thái xe ────────────────────────────────────────────
        private CarState currentState = CarState.UNKNOWN;
        private int      stateChangeCount = 0;

        // ─── Dữ liệu quãng đường ──────────────────────────────────────
        private readonly List<PointF> pathPoints = new();   // mm
        private float  robotX       = 0f;
        private float  robotY       = 0f;
        private float  robotHeading = 0f;   // radian

        // ─── Bộ vẽ đường ──────────────────────────────────────────────
        private Bitmap    pathBitmap;
        private Graphics  pathGraphics;
        private const int PATH_PANEL_W = 500;
        private const int PATH_PANEL_H = 500;
        private float     mapScale  = 1.0f;   // pixel/mm, tự động
        private PointF    mapOffset = new(PATH_PANEL_W / 2f, PATH_PANEL_H / 2f);

        // ─── Enum ─────────────────────────────────────────────────────
        public enum CarState { STOP, FOLLOW_LINE, TURN_LEFT, TURN_RIGHT, LOST_LINE, UNKNOWN }

        // ==============================================================
        // Khởi tạo
        // ==============================================================
        public Form1()
        {
            InitializeComponent();
            SetupSerialPort();
            SetupPathCanvas();
            SetupUpdateTimer();
        }

        private void SetupSerialPort()
        {
            serialPort = new SerialPort
            {
                BaudRate = 115200,
                DataBits = 8,
                StopBits = StopBits.One,
                Parity   = Parity.None,
                NewLine  = "\r\n"
            };
            serialPort.DataReceived += OnDataReceived;
        }

        private void SetupPathCanvas()
        {
            pathBitmap   = new Bitmap(PATH_PANEL_W, PATH_PANEL_H);
            pathGraphics = Graphics.FromImage(pathBitmap);
            pathGraphics.SmoothingMode = SmoothingMode.AntiAlias;
            ClearCanvas();
        }

        private void SetupUpdateTimer()
        {
            var timer = new Timer { Interval = 100 };  // 10 Hz UI refresh
            timer.Tick += (_, _) => UpdateUI();
            timer.Start();
        }

        // ==============================================================
        // Nhận dữ liệu UART
        // ==============================================================
        private void OnDataReceived(object sender, SerialDataReceivedEventArgs e)
        {
            try
            {
                receivedBuffer += serialPort.ReadExisting();
                ParseBuffer();
            }
            catch { /* bỏ qua lỗi đọc serial */ }
        }

        private void ParseBuffer()
        {
            // Tìm tất cả frame hoàn chỉnh dạng $...,..#
            var stateRx = new Regex(@"\$STATE,([A-Z_]+)#");
            var posRx   = new Regex(@"\$POS,(-?\d+\.?\d*),(-?\d+\.?\d*),(-?\d+\.?\d*)#");

            // --- Parse STATE ---
            foreach (Match m in stateRx.Matches(receivedBuffer))
            {
                var newState = m.Groups[1].Value switch
                {
                    "STOP"        => CarState.STOP,
                    "FOLLOW_LINE" => CarState.FOLLOW_LINE,
                    "TURN_LEFT"   => CarState.TURN_LEFT,
                    "TURN_RIGHT"  => CarState.TURN_RIGHT,
                    "LOST_LINE"   => CarState.LOST_LINE,
                    _             => CarState.UNKNOWN
                };
                if (newState != currentState)
                {
                    currentState = newState;
                    stateChangeCount++;
                }
            }

            // --- Parse POS ---
            foreach (Match m in posRx.Matches(receivedBuffer))
            {
                if (float.TryParse(m.Groups[1].Value, out float x) &&
                    float.TryParse(m.Groups[2].Value, out float y) &&
                    float.TryParse(m.Groups[3].Value, out float h))
                {
                    robotX       = x;
                    robotY       = y;
                    robotHeading = h;
                    AddPathPoint(x, y);
                }
            }

            // Xóa phần đã parse, giữ lại phần chưa hoàn chỉnh
            int lastHash = receivedBuffer.LastIndexOf('#');
            if (lastHash >= 0)
                receivedBuffer = receivedBuffer[(lastHash + 1)..];

            // Giới hạn buffer tránh tràn bộ nhớ
            if (receivedBuffer.Length > 512)
                receivedBuffer = receivedBuffer[^256..];
        }

        // ==============================================================
        // Vẽ đường đi
        // ==============================================================
        private void AddPathPoint(float xMm, float yMm)
        {
            lock (pathPoints)
            {
                pathPoints.Add(new PointF(xMm, yMm));

                // Giới hạn số điểm lưu (bộ nhớ)
                if (pathPoints.Count > 5000)
                    pathPoints.RemoveAt(0);
            }
        }

        private void RedrawPath()
        {
            lock (pathPoints)
            {
                ClearCanvas();
                if (pathPoints.Count < 2) return;

                // Tính scale tự động để vừa khung
                AutoScale();

                // Vẽ lưới
                DrawGrid();

                // Vẽ đường đi
                using var pathPen = new Pen(Color.DodgerBlue, 2f);
                for (int i = 1; i < pathPoints.Count; i++)
                {
                    var p0 = WorldToScreen(pathPoints[i - 1]);
                    var p1 = WorldToScreen(pathPoints[i]);
                    pathGraphics.DrawLine(pathPen, p0, p1);
                }

                // Vẽ điểm xuất phát (màu xanh lá)
                var start = WorldToScreen(pathPoints[0]);
                pathGraphics.FillEllipse(Brushes.LimeGreen, start.X - 6, start.Y - 6, 12, 12);
                pathGraphics.DrawEllipse(Pens.DarkGreen,    start.X - 6, start.Y - 6, 12, 12);

                // Vẽ vị trí hiện tại (màu đỏ + mũi tên hướng)
                var curr = WorldToScreen(pathPoints[^1]);
                pathGraphics.FillEllipse(Brushes.OrangeRed, curr.X - 7, curr.Y - 7, 14, 14);
                DrawHeadingArrow(curr, robotHeading);
            }

            // Yêu cầu vẽ lại PictureBox trên UI thread
            if (pictureBoxPath.InvokeRequired)
                pictureBoxPath.Invoke(() => pictureBoxPath.Invalidate());
            else
                pictureBoxPath.Invalidate();
        }

        private void AutoScale()
        {
            if (pathPoints.Count == 0) return;

            float minX = float.MaxValue, maxX = float.MinValue;
            float minY = float.MaxValue, maxY = float.MinValue;
            foreach (var p in pathPoints)
            {
                if (p.X < minX) minX = p.X;
                if (p.X > maxX) maxX = p.X;
                if (p.Y < minY) minY = p.Y;
                if (p.Y > maxY) maxY = p.Y;
            }

            float rangeX = maxX - minX + 200;  // +200mm padding
            float rangeY = maxY - minY + 200;
            float scaleX = (PATH_PANEL_W - 40) / rangeX;
            float scaleY = (PATH_PANEL_H - 40) / rangeY;
            mapScale = Math.Min(scaleX, scaleY);
            if (mapScale < 0.1f) mapScale = 0.1f;

            // Tâm bản đồ ở giữa vùng vẽ
            float centerX = (minX + maxX) / 2f;
            float centerY = (minY + maxY) / 2f;
            mapOffset = new PointF(
                PATH_PANEL_W / 2f - centerX * mapScale,
                PATH_PANEL_H / 2f + centerY * mapScale   // Y lật vì screen Y đi xuống
            );
        }

        private PointF WorldToScreen(PointF world)
        {
            // Hệ tọa độ robot: X = phải, Y = lên
            // Hệ tọa độ màn hình: X = phải, Y = xuống → lật Y
            return new PointF(
                world.X *  mapScale + mapOffset.X,
                world.Y * -mapScale + mapOffset.Y
            );
        }

        private void DrawGrid()
        {
            using var gridPen = new Pen(Color.FromArgb(40, 0, 0, 0), 1f);
            gridPen.DashStyle = DashStyle.Dot;

            // Vẽ lưới mỗi 100mm (thực tế)
            float gridMm   = 100f;
            float gridPx   = gridMm * mapScale;
            if (gridPx < 20) gridPx = 20;  // tối thiểu 20px

            for (float x = mapOffset.X % gridPx; x < PATH_PANEL_W; x += gridPx)
                pathGraphics.DrawLine(gridPen, x, 0, x, PATH_PANEL_H);
            for (float y = mapOffset.Y % gridPx; y < PATH_PANEL_H; y += gridPx)
                pathGraphics.DrawLine(gridPen, 0, y, PATH_PANEL_W, y);

            // Trục gốc (x=0, y=0) in đậm hơn
            var origin = WorldToScreen(new PointF(0, 0));
            pathGraphics.DrawLine(Pens.Gray, origin.X, 0, origin.X, PATH_PANEL_H);
            pathGraphics.DrawLine(Pens.Gray, 0, origin.Y, PATH_PANEL_W, origin.Y);
        }

        private void DrawHeadingArrow(PointF pos, float headingRad)
        {
            float len = 20f;
            float dx  =  MathF.Cos(headingRad) * len;
            float dy  = -MathF.Sin(headingRad) * len;   // lật Y
            using var arrowPen = new Pen(Color.Red, 2f)
            {
                EndCap = LineCap.ArrowAnchor
            };
            pathGraphics.DrawLine(arrowPen, pos.X, pos.Y, pos.X + dx, pos.Y + dy);
        }

        private void ClearCanvas()
        {
            pathGraphics.Clear(Color.WhiteSmoke);
        }

        // ==============================================================
        // Cập nhật UI (timer 100ms)
        // ==============================================================
        private void UpdateUI()
        {
            if (InvokeRequired) { Invoke(UpdateUI); return; }

            // Trạng thái
            (labelStatus.Text, labelStatus.ForeColor) = currentState switch
            {
                CarState.STOP        => ("● Dừng lại",              Color.Red),
                CarState.FOLLOW_LINE => ("▶ Đang đi thẳng",         Color.Green),
                CarState.TURN_LEFT   => ("◀ Đang rẽ trái",          Color.DarkOrange),
                CarState.TURN_RIGHT  => ("▶ Đang rẽ phải",          Color.DarkOrange),
                CarState.LOST_LINE   => ("? Mất line — đang tìm",   Color.DodgerBlue),
                _                   => ("— Chưa kết nối",           Color.Gray),
            };

            // Tọa độ
            labelCoord.Text = $"X={robotX:F0} mm   Y={robotY:F0} mm   θ={robotHeading * 180f / MathF.PI:F1}°";

            labelStateCount.Text = $"Số lần đổi trạng thái: {stateChangeCount}";

            // Vẽ lại bản đồ
            RedrawPath();
        }

        // ==============================================================
        // Sự kiện nút bấm
        // ==============================================================
        private void buttonConnect_Click(object sender, EventArgs e)
        {
            if (serialPort.IsOpen)
            {
                serialPort.Close();
                buttonConnect.Text   = "Kết nối";
                buttonStart.Enabled  = false;
                buttonStop.Enabled   = false;
                buttonReset.Enabled  = false;
                comboBoxPort.Enabled = true;
            }
            else
            {
                string port = comboBoxPort.SelectedItem?.ToString();
                if (string.IsNullOrEmpty(port)) { MessageBox.Show("Chọn cổng COM trước!"); return; }
                try
                {
                    serialPort.PortName = port;
                    serialPort.Open();
                    buttonConnect.Text   = "Ngắt kết nối";
                    buttonStart.Enabled  = true;
                    buttonStop.Enabled   = true;
                    buttonReset.Enabled  = true;
                    comboBoxPort.Enabled = false;
                }
                catch (Exception ex) { MessageBox.Show("Lỗi kết nối: " + ex.Message); }
            }
        }

        private void buttonStart_Click(object sender, EventArgs e)
        {
            if (serialPort.IsOpen) serialPort.Write("G");
        }

        private void buttonStop_Click(object sender, EventArgs e)
        {
            if (serialPort.IsOpen) serialPort.Write("S");
        }

        private void buttonReset_Click(object sender, EventArgs e)
        {
            // Gửi lệnh reset tọa độ xuống xe
            if (serialPort.IsOpen) serialPort.Write("R");

            // Xóa đường vẽ trên PC
            lock (pathPoints) pathPoints.Clear();
            robotX = robotY = robotHeading = 0f;
            ClearCanvas();
            pictureBoxPath.Invalidate();
        }

        private void buttonSavePath_Click(object sender, EventArgs e)
        {
            using var dlg = new SaveFileDialog
            {
                Filter   = "PNG Image|*.png",
                FileName = $"path_{DateTime.Now:yyyyMMdd_HHmmss}.png"
            };
            if (dlg.ShowDialog() == DialogResult.OK)
                pathBitmap.Save(dlg.FileName);
        }

        // ==============================================================
        // PictureBox Paint — hiển thị bitmap đã vẽ
        // ==============================================================
        private void pictureBoxPath_Paint(object sender, PaintEventArgs e)
        {
            e.Graphics.DrawImage(pathBitmap, 0, 0);

            // Vẽ chú thích
            e.Graphics.DrawString("● Xuất phát", new Font("Segoe UI", 8), Brushes.DarkGreen, 5, 5);
            e.Graphics.DrawString("● Vị trí hiện tại", new Font("Segoe UI", 8), Brushes.OrangeRed, 5, 20);
        }

        // ==============================================================
        // Form Load & Close
        // ==============================================================
        private void Form1_Load(object sender, EventArgs e)
        {
            var ports = SerialPort.GetPortNames();
            comboBoxPort.Items.AddRange(ports);
            if (ports.Length > 0) comboBoxPort.SelectedIndex = 0;

            buttonStart.Enabled = false;
            buttonStop.Enabled  = false;
            buttonReset.Enabled = false;
        }

        private void Form1_FormClosing(object sender, FormClosingEventArgs e)
        {
            if (serialPort.IsOpen) serialPort.Close();
            serialPort.Dispose();
            pathGraphics.Dispose();
            pathBitmap.Dispose();
        }

        // ==============================================================
        // InitializeComponent — định nghĩa layout
        // ==============================================================
        private void InitializeComponent()
        {
            this.comboBoxPort     = new ComboBox();
            this.buttonConnect    = new Button();
            this.buttonStart      = new Button();
            this.buttonStop       = new Button();
            this.buttonReset      = new Button();
            this.buttonSavePath   = new Button();
            this.labelStatus      = new Label();
            this.labelCoord       = new Label();
            this.labelStateCount  = new Label();
            this.labelPortSelect  = new Label();
            this.pictureBoxPath   = new PictureBox();

            this.SuspendLayout();

            // ── Cột trái: điều khiển (chiều rộng 260px) ──────────────

            this.labelPortSelect.SetBounds(10, 10, 120, 18);
            this.labelPortSelect.Text = "Chọn cổng COM:";

            this.comboBoxPort.SetBounds(10, 30, 150, 23);

            this.buttonConnect.SetBounds(165, 30, 85, 23);
            this.buttonConnect.Text   = "Kết nối";
            this.buttonConnect.Click += buttonConnect_Click;

            this.labelStatus.SetBounds(10, 70, 240, 28);
            this.labelStatus.Font = new Font("Segoe UI", 12f, FontStyle.Bold);
            this.labelStatus.Text = "— Chưa kết nối";
            this.labelStatus.AutoSize = false;

            this.labelCoord.SetBounds(10, 105, 240, 18);
            this.labelCoord.Text      = "X=0 mm   Y=0 mm   θ=0.0°";
            this.labelCoord.ForeColor = Color.Navy;

            // Nút START
            this.buttonStart.SetBounds(10, 140, 110, 44);
            this.buttonStart.Text      = "▶ START";
            this.buttonStart.BackColor = Color.LimeGreen;
            this.buttonStart.Font      = new Font("Segoe UI", 11f, FontStyle.Bold);
            this.buttonStart.Click    += buttonStart_Click;

            // Nút STOP
            this.buttonStop.SetBounds(130, 140, 110, 44);
            this.buttonStop.Text      = "■ STOP";
            this.buttonStop.BackColor = Color.OrangeRed;
            this.buttonStop.ForeColor = Color.White;
            this.buttonStop.Font      = new Font("Segoe UI", 11f, FontStyle.Bold);
            this.buttonStop.Click    += buttonStop_Click;

            // Nút RESET tọa độ
            this.buttonReset.SetBounds(10, 200, 110, 30);
            this.buttonReset.Text      = "↺ Reset đường";
            this.buttonReset.BackColor = Color.DodgerBlue;
            this.buttonReset.ForeColor = Color.White;
            this.buttonReset.Click    += buttonReset_Click;

            // Nút Lưu ảnh
            this.buttonSavePath.SetBounds(130, 200, 110, 30);
            this.buttonSavePath.Text   = "💾 Lưu ảnh";
            this.buttonSavePath.Click += buttonSavePath_Click;

            this.labelStateCount.SetBounds(10, 250, 240, 18);
            this.labelStateCount.Text = "Số lần đổi trạng thái: 0";

            // ── Cột phải: bản đồ đường đi ────────────────────────────
            this.pictureBoxPath.SetBounds(260, 5, PATH_PANEL_W, PATH_PANEL_H);
            this.pictureBoxPath.BorderStyle = BorderStyle.FixedSingle;
            this.pictureBoxPath.BackColor   = Color.WhiteSmoke;
            this.pictureBoxPath.Paint      += pictureBoxPath_Paint;

            // ── Form ──────────────────────────────────────────────────
            this.ClientSize = new Size(PATH_PANEL_W + 270, PATH_PANEL_H + 10);
            this.Text       = "Line Following Robot — Path Visualizer";
            this.FormBorderStyle = FormBorderStyle.FixedSingle;
            this.MaximizeBox    = false;

            this.Controls.AddRange(new Control[] {
                labelPortSelect, comboBoxPort, buttonConnect,
                labelStatus, labelCoord,
                buttonStart, buttonStop, buttonReset, buttonSavePath,
                labelStateCount, pictureBoxPath
            });

            this.Load         += Form1_Load;
            this.FormClosing  += Form1_FormClosing;
            this.ResumeLayout(false);
        }

        // ─── Khai báo controls ────────────────────────────────────────
        private ComboBox   comboBoxPort;
        private Button     buttonConnect, buttonStart, buttonStop,
                           buttonReset, buttonSavePath;
        private Label      labelStatus, labelCoord,
                           labelStateCount, labelPortSelect;
        private PictureBox pictureBoxPath;
    }
}