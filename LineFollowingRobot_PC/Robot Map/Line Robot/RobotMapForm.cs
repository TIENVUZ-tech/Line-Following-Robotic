using System;
using System.Collections.Generic;
using System.Drawing;
using System.Drawing.Drawing2D;
using System.Globalization;
using System.Windows.Forms;

namespace BleUartReader
{
    class RobotMapForm : Form
    {
        private readonly List<PointF> _trail = new();
        private readonly object _lock = new();

        private string _state = "????";
        private string _sns = "00000";
        private float _error;
        private int _leftPwm, _rightPwm;
        private float _currentX, _currentY, _currentHeading;
        private bool _hasData;

        private const float PixelsPerMeter = 100f;
        private const float RobotRadius = 6f;
        private const float ArrowLength = 22f;

        private float _zoom = 1f;
        private PointF _pan = PointF.Empty;
        private Point _dragStart;
        private PointF _panAtDragStart;
        private bool _isDragging;

        public RobotMapForm(BleUartClient client)
        {
            Text = "Robot 2D Map";
            Size = new Size(900, 700);
            DoubleBuffered = true;
            BackColor = Color.FromArgb(18, 18, 22);
            ForeColor = Color.White;
            SetStyle(ControlStyles.AllPaintingInWmPaint |
            ControlStyles.UserPaint |
            ControlStyles.OptimizedDoubleBuffer,
            true);

            MouseWheel += OnMouseWheel;
            MouseDown += OnMouseDown;
            MouseMove += OnMouseMove;
            MouseUp += OnMouseUp;

            client.DataReceived += OnDataReceived;
        }

        private void OnMouseWheel(object? sender, MouseEventArgs e)
        {
            float factor = e.Delta > 0 ? 1.1f : 1f / 1.1f;
            float cx = ClientSize.Width / 2f;
            float cy = ClientSize.Height / 2f;

            float scale = PixelsPerMeter * _zoom;
            float wx = (e.X - cx - _pan.X) / scale;
            float wy = -(e.Y - cy - _pan.Y) / scale;

            _zoom = Math.Clamp(_zoom * factor, 0.05f, 50f);
            float newScale = PixelsPerMeter * _zoom;

            _pan = new PointF(
                e.X - cx - wx * newScale,
                e.Y - cy + wy * newScale);

            Invalidate();
        }

        private void OnMouseDown(object? sender, MouseEventArgs e)
        {
            if (e.Button == MouseButtons.Left)
            {
                _isDragging = true;
                _dragStart = e.Location;
                _panAtDragStart = _pan;
                Cursor = Cursors.Hand;
            }
        }

        private void OnMouseMove(object? sender, MouseEventArgs e)
        {
            if (_isDragging)
            {
                _pan = new PointF(
                    _panAtDragStart.X + (e.X - _dragStart.X),
                    _panAtDragStart.Y + (e.Y - _dragStart.Y));
                Invalidate();
            }
        }

        private void OnMouseUp(object? sender, MouseEventArgs e)
        {
            if (e.Button == MouseButtons.Left && _isDragging)
            {
                _isDragging = false;
                Cursor = Cursors.Default;
            }
        }

        // Expected format: "ST:FOLW SNS:10110 ERR:-2.0 L:420 R:480 X:1.234 Y:0.056 H:314\r\n"
        private void OnDataReceived(object? sender, string data)
        {
            string state = "????";
            string sns = "00000";
            float error = 0, x = 0, y = 0, heading = 0;
            int leftPwm = 0, rightPwm = 0;
            bool gotX = false, gotY = false, gotH = false;

            foreach (string token in data.Trim().Split(' '))
            {
                if (token.StartsWith("ST:"))
                    state = token[3..];
                else if (token.StartsWith("SNS:"))
                    sns = token[4..];
                else if (token.StartsWith("ERR:") && float.TryParse(token.AsSpan(4), NumberStyles.Float, CultureInfo.InvariantCulture, out float pe))
                    error = pe;
                else if (token.StartsWith("L:") && int.TryParse(token.AsSpan(2), out int pl))
                    leftPwm = pl;
                else if (token.StartsWith("R:") && int.TryParse(token.AsSpan(2), out int pr))
                    rightPwm = pr;
                else if (token.StartsWith("X:") && float.TryParse(token.AsSpan(2), NumberStyles.Float, CultureInfo.InvariantCulture, out float px))
                    { x = px; gotX = true; }
                else if (token.StartsWith("Y:") && float.TryParse(token.AsSpan(2), NumberStyles.Float, CultureInfo.InvariantCulture, out float py))
                    { y = py; gotY = true; }
                else if (token.StartsWith("H:") && float.TryParse(token.AsSpan(2), NumberStyles.Float, CultureInfo.InvariantCulture, out float ph))
                    { heading = ph; gotH = true; }
            }

            if (!gotX || !gotY || !gotH) return;

            lock (_lock)
            {
                _state = state;
                _sns = sns;
                _error = error;
                _leftPwm = leftPwm;
                _rightPwm = rightPwm;
                _currentX = x;
                _currentY = y;
                _currentHeading = heading;
                _trail.Add(new PointF(x, y));
                _hasData = true;
            }

            if (InvokeRequired)
                Invoke((Action)Invalidate);
            else
                Invalidate();
        }

        protected override void OnPaint(PaintEventArgs e)
        {
            base.OnPaint(e);

            Graphics g = e.Graphics;
            g.SmoothingMode = SmoothingMode.AntiAlias;

            float cx = ClientSize.Width / 2f;
            float cy = ClientSize.Height / 2f;

            DrawGrid(g, cx, cy);
            DrawAxisLabels(g, cx, cy);

            PointF[] snapshot;
            float snapX, snapY, snapHeading;
            string snapState, snapSns;
            float snapError;
            int snapL, snapR;
            bool hasData;

            lock (_lock)
            {
                snapshot = _trail.ToArray();
                snapX = _currentX;
                snapY = _currentY;
                snapHeading = _currentHeading;
                snapState = _state;
                snapSns = _sns;
                snapError = _error;
                snapL = _leftPwm;
                snapR = _rightPwm;
                hasData = _hasData;
            }

            if (snapshot.Length > 1)
            {
                var screenPoints = new PointF[snapshot.Length];
                for (int i = 0; i < snapshot.Length; i++)
                    screenPoints[i] = ToScreen(snapshot[i].X, snapshot[i].Y, cx, cy);

                using var glowPen = new Pen(Color.FromArgb(60, 120, 200, 255), 8f)
                {
                    StartCap = LineCap.Round,
                    EndCap = LineCap.Round,
                    LineJoin = LineJoin.Round
                };

                using var trailPen = new Pen(Color.FromArgb(0, 220, 255), 2.8f)
                {
                    StartCap = LineCap.Round,
                    EndCap = LineCap.Round,
                    LineJoin = LineJoin.Round
                };

                g.DrawLines(glowPen, screenPoints);
                g.DrawLines(trailPen, screenPoints);
            }

            if (hasData)
                DrawRobot(g, snapX, snapY, snapHeading, cx, cy);

            DrawStatsPanel(g, snapState, snapSns, snapError, snapL, snapR, snapX, snapY, snapHeading, hasData);

            using var hudFont = new Font("Consolas", 9f);
            using var hudBrush = new SolidBrush(Color.FromArgb(140, 220, 220, 220));

            g.DrawString(
                $"Trail Points: {snapshot.Length}\nPan: {_pan.X:F0}, {_pan.Y:F0}",
                hudFont,
                hudBrush,
                12,
                ClientSize.Height - 50);
        }

        private void DrawGrid(Graphics g, float cx, float cy)
        {
            float step = PixelsPerMeter * _zoom;

            float originX = cx + _pan.X;
            float originY = cy + _pan.Y;

            using var minorPen = new Pen(Color.FromArgb(28, 28, 35), 1f);
            using var majorPen = new Pen(Color.FromArgb(45, 45, 58), 1.2f);
            using var axisPen  = new Pen(Color.FromArgb(90, 160, 255), 2f);

            float startX = originX % step;
            if (startX < 0) startX += step;

            int index = 0;
            for (float x = startX; x < ClientSize.Width; x += step)
            {
                g.DrawLine(index % 5 == 0 ? majorPen : minorPen,
                        x, 0, x, ClientSize.Height);
                index++;
            }

            float startY = originY % step;
            if (startY < 0) startY += step;

            index = 0;
            for (float y = startY; y < ClientSize.Height; y += step)
            {
                g.DrawLine(index % 5 == 0 ? majorPen : minorPen,
                        0, y, ClientSize.Width, y);
                index++;
            }

            g.DrawLine(axisPen, originX, 0, originX, ClientSize.Height);
            g.DrawLine(axisPen, 0, originY, ClientSize.Width, originY);
        }

        private void DrawAxisLabels(Graphics g, float cx, float cy)
        {
            using var font = new Font("Segoe UI", 7f);
            using var brush = new SolidBrush(Color.Gray);

            float step = PixelsPerMeter * _zoom;
            float originX = cx + _pan.X;
            float originY = cy + _pan.Y;
            int range = (int)(Math.Max(ClientSize.Width, ClientSize.Height) / step) + 2;

            for (int i = -range; i <= range; i++)
            {
                if (i == 0) continue;
                float sx = originX + i * step;
                float sy = originY - i * step;

                if (sx >= 0 && sx < ClientSize.Width)
                    g.DrawString($"{i}m", font, brush, sx + 2, originY + 2);
                if (sy >= 0 && sy < ClientSize.Height)
                    g.DrawString($"{i}m", font, brush, originX + 2, sy + 2);
            }
        }

        private void DrawRobot(Graphics g, float x, float y, float heading, float cx, float cy)
        {
            PointF pos = ToScreen(x, y, cx, cy);

            float robotR = Math.Max(RobotRadius * _zoom, 6f);
            float arrowLen = Math.Max(ArrowLength * _zoom, 18f);

            float dx = (float)Math.Cos(heading) * arrowLen;
            float dy = -(float)Math.Sin(heading) * arrowLen;

            RectangleF body = new RectangleF(
                pos.X - robotR,
                pos.Y - robotR,
                robotR * 2,
                robotR * 2);

            using var glowBrush = new SolidBrush(Color.FromArgb(80, 0, 255, 255));
            using var bodyBrush = new LinearGradientBrush(
                body,
                Color.FromArgb(0, 255, 255),
                Color.FromArgb(0, 120, 255),
                45f);

            using var borderPen = new Pen(Color.White, 1.5f);

            using var arrowPen = new Pen(Color.White, 3f)
            {
                CustomEndCap = new AdjustableArrowCap(5, 5)
            };

            g.FillEllipse(glowBrush,
                body.X - 6,
                body.Y - 6,
                body.Width + 12,
                body.Height + 12);

            g.FillEllipse(bodyBrush, body);
            g.DrawEllipse(borderPen, body.X, body.Y, body.Width, body.Height);

            g.DrawLine(arrowPen,
                pos.X,
                pos.Y,
                pos.X + dx,
                pos.Y + dy);
        }

        private void DrawStatsPanel(Graphics g, string state, string sns, float error,
            int leftPwm, int rightPwm, float x, float y, float heading, bool hasData)
        {
            using var titleFont = new Font("Segoe UI", 9f, FontStyle.Bold);
            using var valueFont = new Font("Segoe UI", 9f);
            using var smallFont = new Font("Segoe UI", 7.5f);
            using var bg = new SolidBrush(Color.FromArgb(220, 20, 24, 30));
            using var fgLight = new SolidBrush(Color.FromArgb(220, 220, 220));
            using var fgDim = new SolidBrush(Color.FromArgb(150, 150, 150));

            const int panelX = 8;
            const int panelY = 8;
            const int panelW = 270;
            const int lineH = 20;
            const int snsRowH = 30; // taller to fit dot + index number below
            const int padX = 10;
            const int padY = 8;
            int totalH = padY * 2 + lineH * 5 + snsRowH + 6;

            using GraphicsPath path = new GraphicsPath();

            int radius = 16;

            path.AddArc(panelX, panelY, radius, radius, 180, 90);
            path.AddArc(panelX + panelW - radius, panelY, radius, radius, 270, 90);
            path.AddArc(panelX + panelW - radius, panelY + totalH - radius, radius, radius, 0, 90);
            path.AddArc(panelX, panelY + totalH - radius, radius, radius, 90, 90);
            path.CloseFigure();

            g.FillPath(bg, path);

            float fy = panelY + padY;

            // ── Row 1: State badge ──────────────────────────────────────────
            Color stateColor = state switch
            {
                "FOLW" => Color.FromArgb(255, 80, 200, 100),
                "IDLE" => Color.FromArgb(255, 160, 160, 160),
                "LOST" => Color.FromArgb(255, 255, 165, 0),
                "STOP" => Color.FromArgb(255, 220, 60, 60),
                _      => Color.FromArgb(255, 180, 100, 220),
            };
            string stateLabel = state switch
            {
                "FOLW" => "FOLLOWING",
                "IDLE" => "IDLE",
                "LOST" => "LINE LOST",
                "STOP" => "STOPPED",
                _      => "UNKNOWN",
            };

            using var stateBg = new SolidBrush(Color.FromArgb(60, stateColor));
            using var stateFg = new SolidBrush(stateColor);
            using var stateBorder = new Pen(stateColor, 1.5f);
            SizeF stateSize = g.MeasureString(stateLabel, titleFont);
            var stateBadge = new RectangleF(panelX + padX, fy, stateSize.Width + 12, lineH - 2);
            g.FillRectangle(stateBg, stateBadge);
            g.DrawRectangle(stateBorder, stateBadge.X, stateBadge.Y, stateBadge.Width, stateBadge.Height);
            g.DrawString(stateLabel, titleFont, stateFg, stateBadge.X + 6, stateBadge.Y + 2);

            // Zoom on same row, right-aligned
            string zoomStr = $"Zoom: {_zoom:F2}x";
            SizeF zoomSize = g.MeasureString(zoomStr, smallFont);
            g.DrawString(zoomStr, smallFont, fgDim, panelX + panelW - padX - zoomSize.Width, fy + 3);
            fy += lineH + 4;

            // ── Row 2: IR Sensors (SNS) ─────────────────────────────────────
            g.DrawString("SNS", smallFont, fgDim, panelX + padX, fy + 3);
            float dotR = 9f;
            float dotSpacing = dotR * 2 + 4;
            float dotX = panelX + padX + 36;
            float dotY = fy + 2;
            for (int i = 0; i < sns.Length && i < 5; i++)
            {
                float cx2 = dotX + i * dotSpacing;
                bool on = sns[i] == '1';
                Color dotColor = on ? Color.FromArgb(255, 255, 220, 50) : Color.FromArgb(255, 70, 70, 70);
                using var dotBrush = new SolidBrush(dotColor);
                g.FillEllipse(dotBrush, cx2, dotY, dotR * 2, dotR * 2);

                // sensor index label (1-based) centred under the dot
                string label = (i + 1).ToString();
                SizeF ls = g.MeasureString(label, smallFont);
                using var idxBrush = new SolidBrush(on
                    ? Color.FromArgb(255, 200, 170, 30)
                    : Color.FromArgb(255, 100, 100, 100));
                g.DrawString(label, smallFont, idxBrush,
                    cx2 + dotR - ls.Width / 2f, dotY + dotR * 2 + 1);
            }
            fy += snsRowH;

            if (!hasData)
            {
                g.DrawString("Waiting for data...", valueFont, fgDim, panelX + padX, fy + 2);
                return;
            }

            // ── Row 3: ERR ──────────────────────────────────────────────────
            Color errColor = Math.Abs(error) < 0.5f
                ? Color.FromArgb(255, 80, 200, 100)
                : Math.Abs(error) < 2f
                    ? Color.FromArgb(255, 255, 200, 50)
                    : Color.FromArgb(255, 220, 80, 80);
            using var errFg = new SolidBrush(errColor);
            g.DrawString("ERR", smallFont, fgDim, panelX + padX, fy + 3);
            g.DrawString($"{error:+0.00;-0.00;0.00}", valueFont, errFg, panelX + padX + 36, fy + 2);
            fy += lineH;

            // ── Row 4: Motor PWM ────────────────────────────────────────────
            g.DrawString("L", smallFont, fgDim, panelX + padX, fy + 3);
            g.DrawString($"{leftPwm}", valueFont, fgLight, panelX + padX + 16, fy + 2);
            DrawMotorBar(g, panelX + padX + 52, fy + 5, 70, 10, leftPwm, Color.CornflowerBlue);

            g.DrawString("R", smallFont, fgDim, panelX + padX + 140, fy + 3);
            g.DrawString($"{rightPwm}", valueFont, fgLight, panelX + padX + 156, fy + 2);
            DrawMotorBar(g, panelX + padX + 192, fy + 5, 70, 10, rightPwm, Color.MediumOrchid);
            fy += lineH;

            // ── Row 5: X / Y position ───────────────────────────────────────
            g.DrawString("X", smallFont, fgDim, panelX + padX, fy + 3);
            g.DrawString($"{x:F3} m", valueFont, fgLight, panelX + padX + 16, fy + 2);
            g.DrawString("Y", smallFont, fgDim, panelX + padX + 110, fy + 3);
            g.DrawString($"{y:F3} m", valueFont, fgLight, panelX + padX + 126, fy + 2);
            fy += lineH;

            // ── Row 6: Heading ──────────────────────────────────────────────
            g.DrawString("H", smallFont, fgDim, panelX + padX, fy + 3);
            g.DrawString($"{heading:F3} rad  ({heading * 180 / Math.PI:F1}°)", valueFont, fgLight, panelX + padX + 16, fy + 2);
        }

        // Draws a small horizontal bar representing a PWM value (0–999 range assumed)
            private static void DrawMotorBar(Graphics g,
            float x,
            float y,
            float maxW,
            float h,
            int value,
            Color color)
        {
            const int maxPwm = 999;

            float ratio = Math.Clamp(Math.Abs(value) / (float)maxPwm, 0f, 1f);
            float filled = ratio * maxW;

            using var bgBrush = new SolidBrush(Color.FromArgb(40, 40, 50));

            using var fgBrush = new LinearGradientBrush(
                new RectangleF(x, y, filled, h),
                color,
                Color.White,
                0f);

            g.FillRectangle(bgBrush, x, y, maxW, h);
            g.FillRectangle(fgBrush, x, y, filled, h);

            using var border = new Pen(Color.FromArgb(80, 80, 90));
            g.DrawRectangle(border, x, y, maxW, h);
        }

        private PointF ToScreen(float x, float y, float cx, float cy)
        {
            float scale = PixelsPerMeter * _zoom;
            return new(cx + _pan.X + x * scale, cy + _pan.Y - y * scale);
        }
    }
}
