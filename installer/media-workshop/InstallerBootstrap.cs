using System;
using System.Diagnostics;
using System.Drawing;
using System.IO;
using System.IO.Compression;
using System.Reflection;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Windows.Forms;

[assembly: AssemblyTitle("OBS 素材工作台安装程序")]
[assembly: AssemblyDescription("OBS Studio 32.2.1 素材工作台完整安装程序")]
[assembly: AssemblyProduct("OBS 素材工作台")]
[assembly: AssemblyVersion("1.0.1.0")]
[assembly: AssemblyFileVersion("1.0.1.0")]

internal static class InstallerBootstrap
{
    private const string PayloadResourceName = "ObsMediaWorkshopPayload";
    private const string ProductName = "OBS 素材工作台";
    private const string DefaultInstallFolderName = "OBS素材工作台";
    private const string ProductVersion = "1.0.1";

    [STAThread]
    private static int Main(string[] args)
    {
        bool unattendedRequested = Array.Exists(args, delegate(string arg) {
            return string.Equals(arg, "--yes", StringComparison.OrdinalIgnoreCase);
        });
        InstallerOptions options;
        try {
            options = InstallerOptions.Parse(args);
        }
        catch (Exception ex) {
            if (unattendedRequested) {
                WriteLog(FindRawArgumentValue(args, "--log"), "安装参数错误：" + ex.Message);
                return 1;
            }
            MessageBox.Show(ex.Message, ProductName + "安装程序", MessageBoxButtons.OK, MessageBoxIcon.Error);
            return 1;
        }

        if (options.Unattended)
            return RunUnattended(options);

        Application.EnableVisualStyles();
        Application.SetCompatibleTextRenderingDefault(false);
        using (InstallerForm form = new InstallerForm(options)) {
            Application.Run(form);
            return form.ExitCode;
        }
    }

    private static int RunUnattended(InstallerOptions options)
    {
        try {
            string installRoot = ValidateInstallRoot(options.InstallRoot);
            InstallationResult result = RunInstallation(options, installRoot);
            return result.ExitCode;
        }
        catch (Exception ex) {
            WriteLog(options.LogPath, "安装失败：" + ex);
            return 1;
        }
    }

    internal static InstallationResult RunInstallation(InstallerOptions options, string installRoot)
    {
        string temporaryDirectory = Path.Combine(
            Path.GetTempPath(), "OBS-Media-Workshop-" + Guid.NewGuid().ToString("N"));
        StringBuilder processLog = new StringBuilder();

        try {
            WriteLog(options.LogPath, "正在准备安装文件...");
            Directory.CreateDirectory(temporaryDirectory);
            ExtractPayload(temporaryDirectory);

            string installScript = Path.Combine(temporaryDirectory, "install.ps1");
            if (!File.Exists(installScript))
                throw new InvalidDataException("安装脚本不存在。");

            ProcessStartInfo startInfo = new ProcessStartInfo();
            startInfo.FileName = Path.Combine(
                Environment.GetFolderPath(Environment.SpecialFolder.System),
                "WindowsPowerShell\\v1.0\\powershell.exe");
            startInfo.Arguments = "-NoProfile -ExecutionPolicy Bypass -File " + Quote(installScript);
            startInfo.WorkingDirectory = temporaryDirectory;
            startInfo.UseShellExecute = false;
            startInfo.CreateNoWindow = true;
            startInfo.RedirectStandardOutput = true;
            startInfo.RedirectStandardError = true;
            startInfo.StandardOutputEncoding = Encoding.UTF8;
            startInfo.StandardErrorEncoding = Encoding.UTF8;
            startInfo.EnvironmentVariables["OBS_MEDIA_WORKSHOP_INSTALL_ROOT"] = installRoot;

            if (!string.IsNullOrWhiteSpace(options.ShortcutRoot))
                startInfo.EnvironmentVariables["OBS_MEDIA_WORKSHOP_SHORTCUT_ROOT"] =
                    Path.GetFullPath(Environment.ExpandEnvironmentVariables(options.ShortcutRoot));
            if (!string.IsNullOrWhiteSpace(options.LaunchArguments))
                startInfo.EnvironmentVariables["OBS_MEDIA_WORKSHOP_LAUNCH_ARGUMENTS"] = options.LaunchArguments;
            if (options.NoLaunch)
                startInfo.EnvironmentVariables["OBS_MEDIA_WORKSHOP_NO_LAUNCH"] = "1";

            WriteLog(options.LogPath, "正在安装到：" + installRoot);
            using (Process process = new Process()) {
                process.StartInfo = startInfo;
                process.OutputDataReceived += delegate(object sender, DataReceivedEventArgs eventArgs) {
                    if (eventArgs.Data != null) {
                        lock (processLog)
                            processLog.AppendLine(eventArgs.Data);
                        WriteLog(options.LogPath, eventArgs.Data);
                    }
                };
                process.ErrorDataReceived += delegate(object sender, DataReceivedEventArgs eventArgs) {
                    if (eventArgs.Data != null) {
                        lock (processLog)
                            processLog.AppendLine(eventArgs.Data);
                        WriteLog(options.LogPath, eventArgs.Data);
                    }
                };

                if (!process.Start())
                    throw new InvalidOperationException("无法启动 Windows PowerShell。");
                process.BeginOutputReadLine();
                process.BeginErrorReadLine();
                process.WaitForExit();
                process.WaitForExit();

                string output;
                lock (processLog)
                    output = processLog.ToString();

                if (process.ExitCode != 0)
                    return new InstallationResult(process.ExitCode, output);

                WriteLog(options.LogPath, "安装程序执行完成。");
                return new InstallationResult(0, output);
            }
        }
        catch (Exception ex) {
            WriteLog(options.LogPath, "安装失败：" + ex);
            return new InstallationResult(1, ex.Message);
        }
        finally {
            DeleteTemporaryDirectory(temporaryDirectory);
        }
    }

    internal static string ValidateInstallRoot(string value)
    {
        if (string.IsNullOrWhiteSpace(value))
            throw new ArgumentException("请选择安装目录。");

        string expanded = Environment.ExpandEnvironmentVariables(value.Trim().Trim('"'));
        string fullPath = Path.GetFullPath(expanded).TrimEnd(Path.DirectorySeparatorChar, Path.AltDirectorySeparatorChar);
        string pathRoot = Path.GetPathRoot(fullPath);
        if (string.IsNullOrEmpty(pathRoot) ||
            string.Equals(fullPath, pathRoot.TrimEnd(Path.DirectorySeparatorChar), StringComparison.OrdinalIgnoreCase))
            throw new ArgumentException("不能直接安装到磁盘根目录，请选择一个专用文件夹。");

        string windowsPath = Environment.GetFolderPath(Environment.SpecialFolder.Windows)
            .TrimEnd(Path.DirectorySeparatorChar, Path.AltDirectorySeparatorChar);
        if (string.Equals(fullPath, windowsPath, StringComparison.OrdinalIgnoreCase) ||
            fullPath.StartsWith(windowsPath + Path.DirectorySeparatorChar, StringComparison.OrdinalIgnoreCase))
            throw new ArgumentException("不能安装到 Windows 系统目录。");

        string managedInstall = Path.Combine(fullPath, "obs-studio");
        if (File.Exists(managedInstall))
            throw new ArgumentException("安装目录中的 obs-studio 不是文件夹。");
        if (Directory.Exists(managedInstall) &&
            Directory.GetFileSystemEntries(managedInstall).Length > 0 &&
            !File.Exists(Path.Combine(managedInstall, "bin", "64bit", "obs64.exe")))
            throw new ArgumentException(
                "所选目录下已有非本软件管理的 obs-studio 文件夹。请更换目录，避免覆盖其他文件。");

        return fullPath;
    }

    private static void ExtractPayload(string destinationRoot)
    {
        Assembly assembly = Assembly.GetExecutingAssembly();
        using (Stream payload = assembly.GetManifestResourceStream(PayloadResourceName)) {
            if (payload == null)
                throw new InvalidDataException("安装数据资源不存在。");

            using (ZipArchive archive = new ZipArchive(payload, ZipArchiveMode.Read, false)) {
                string safeRoot = Path.GetFullPath(destinationRoot) + Path.DirectorySeparatorChar;
                foreach (ZipArchiveEntry entry in archive.Entries) {
                    string destinationPath = Path.GetFullPath(Path.Combine(destinationRoot, entry.FullName));
                    if (!destinationPath.StartsWith(safeRoot, StringComparison.OrdinalIgnoreCase))
                        throw new InvalidDataException("安装数据包含不安全路径：" + entry.FullName);

                    if (string.IsNullOrEmpty(entry.Name)) {
                        Directory.CreateDirectory(destinationPath);
                        continue;
                    }

                    string parent = Path.GetDirectoryName(destinationPath);
                    if (!string.IsNullOrEmpty(parent))
                        Directory.CreateDirectory(parent);

                    using (Stream input = entry.Open())
                    using (FileStream output = new FileStream(
                        destinationPath, FileMode.Create, FileAccess.Write, FileShare.None))
                        input.CopyTo(output);
                }
            }
        }
    }

    private static string Quote(string value)
    {
        return "\"" + value.Replace("\"", "\\\"") + "\"";
    }

    private static string FindRawArgumentValue(string[] args, string name)
    {
        for (int index = 0; index < args.Length; ++index) {
            if (!string.Equals(args[index], name, StringComparison.OrdinalIgnoreCase))
                continue;
            if (index + 1 >= args.Length)
                return null;

            StringBuilder value = new StringBuilder(args[++index]);
            while (index + 1 < args.Length && !args[index + 1].StartsWith("--", StringComparison.Ordinal))
                value.Append(' ').Append(args[++index]);
            return value.ToString();
        }
        return null;
    }

    private static void DeleteTemporaryDirectory(string path)
    {
        for (int attempt = 0; attempt < 5; ++attempt) {
            try {
                if (Directory.Exists(path))
                    Directory.Delete(path, true);
                return;
            }
            catch {
                Thread.Sleep(500);
            }
        }
    }

    internal static void WriteLog(string logPath, string message)
    {
        if (string.IsNullOrWhiteSpace(logPath))
            return;

        try {
            string fullPath = Path.GetFullPath(Environment.ExpandEnvironmentVariables(logPath));
            string parent = Path.GetDirectoryName(fullPath);
            if (!string.IsNullOrEmpty(parent))
                Directory.CreateDirectory(parent);
            File.AppendAllText(
                fullPath,
                DateTime.Now.ToString("yyyy-MM-dd HH:mm:ss") + " " + message + Environment.NewLine,
                new UTF8Encoding(false));
        }
        catch {
        }
    }

    internal sealed class InstallerOptions
    {
        public bool Unattended;
        public bool NoLaunch;
        public string InstallRoot;
        public string ShortcutRoot;
        public string LaunchArguments;
        public string LogPath;

        public static InstallerOptions Parse(string[] args)
        {
            InstallerOptions options = new InstallerOptions();
            options.InstallRoot = Environment.GetEnvironmentVariable("OBS_MEDIA_WORKSHOP_INSTALL_ROOT");
            if (string.IsNullOrWhiteSpace(options.InstallRoot))
                options.InstallRoot = Path.Combine(
                    Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData), DefaultInstallFolderName);
            options.ShortcutRoot = Environment.GetEnvironmentVariable("OBS_MEDIA_WORKSHOP_SHORTCUT_ROOT");
            options.LaunchArguments = Environment.GetEnvironmentVariable("OBS_MEDIA_WORKSHOP_LAUNCH_ARGUMENTS");
            options.NoLaunch = string.Equals(
                Environment.GetEnvironmentVariable("OBS_MEDIA_WORKSHOP_NO_LAUNCH"),
                "1",
                StringComparison.OrdinalIgnoreCase);

            for (int index = 0; index < args.Length; ++index) {
                string arg = args[index];
                if (string.Equals(arg, "--yes", StringComparison.OrdinalIgnoreCase)) {
                    options.Unattended = true;
                } else if (string.Equals(arg, "--no-launch", StringComparison.OrdinalIgnoreCase)) {
                    options.NoLaunch = true;
                } else if (string.Equals(arg, "--install-root", StringComparison.OrdinalIgnoreCase)) {
                    options.InstallRoot = ReadValue(args, ref index, arg);
                } else if (string.Equals(arg, "--shortcut-root", StringComparison.OrdinalIgnoreCase)) {
                    options.ShortcutRoot = ReadValue(args, ref index, arg);
                } else if (string.Equals(arg, "--launch-arguments", StringComparison.OrdinalIgnoreCase)) {
                    options.LaunchArguments = ReadValue(args, ref index, arg);
                } else if (string.Equals(arg, "--log", StringComparison.OrdinalIgnoreCase)) {
                    options.LogPath = ReadValue(args, ref index, arg);
                } else {
                    throw new ArgumentException("未知安装参数：" + arg);
                }
            }

            return options;
        }

        private static string ReadValue(string[] args, ref int index, string name)
        {
            if (index + 1 >= args.Length)
                throw new ArgumentException(name + " 缺少参数值。");

            StringBuilder value = new StringBuilder(args[++index]);
            while (index + 1 < args.Length && !args[index + 1].StartsWith("--", StringComparison.Ordinal))
                value.Append(' ').Append(args[++index]);
            return value.ToString();
        }
    }

    internal sealed class InstallationResult
    {
        public readonly int ExitCode;
        public readonly string Output;

        public InstallationResult(int exitCode, string output)
        {
            ExitCode = exitCode;
            Output = output ?? string.Empty;
        }
    }

    private sealed class InstallerForm : Form
    {
        private readonly InstallerOptions options;
        private readonly TextBox pathTextBox;
        private readonly Button browseButton;
        private readonly Button installButton;
        private readonly Button cancelButton;
        private readonly CheckBox launchCheckBox;
        private readonly Label targetLabel;
        private readonly Label statusLabel;
        private readonly ProgressBar progressBar;
        private bool installing;

        public int ExitCode { get; private set; }

        public InstallerForm(InstallerOptions options)
        {
            this.options = options;
            ExitCode = 2;

            Text = InstallerBootstrap.ProductName + "安装程序";
            StartPosition = FormStartPosition.CenterScreen;
            ClientSize = new Size(650, 330);
            FormBorderStyle = FormBorderStyle.FixedDialog;
            MaximizeBox = false;
            MinimizeBox = true;
            AutoScaleMode = AutoScaleMode.Dpi;
            Font = new Font("Microsoft YaHei UI", 9F, FontStyle.Regular, GraphicsUnit.Point);

            Label title = new Label();
            title.Text = InstallerBootstrap.ProductName + " " + InstallerBootstrap.ProductVersion;
            title.Font = new Font(Font.FontFamily, 18F, FontStyle.Bold);
            title.AutoSize = true;
            title.Location = new Point(28, 24);
            Controls.Add(title);

            Label subtitle = new Label();
            subtitle.Text = "基于 OBS Studio 32.2.1，已包含素材工作台和媒体播放列表源。";
            subtitle.AutoSize = true;
            subtitle.ForeColor = Color.FromArgb(80, 80, 80);
            subtitle.Location = new Point(31, 66);
            Controls.Add(subtitle);

            Label pathLabel = new Label();
            pathLabel.Text = "安装目录";
            pathLabel.AutoSize = true;
            pathLabel.Location = new Point(31, 108);
            Controls.Add(pathLabel);

            pathTextBox = new TextBox();
            pathTextBox.Location = new Point(31, 132);
            pathTextBox.Size = new Size(500, 28);
            pathTextBox.Text = options.InstallRoot;
            pathTextBox.TextChanged += delegate { UpdateTargetLabel(); };
            Controls.Add(pathTextBox);

            browseButton = new Button();
            browseButton.Text = "浏览...";
            browseButton.Location = new Point(540, 130);
            browseButton.Size = new Size(80, 31);
            browseButton.Click += BrowseButtonClicked;
            Controls.Add(browseButton);

            targetLabel = new Label();
            targetLabel.AutoEllipsis = true;
            targetLabel.ForeColor = Color.FromArgb(90, 90, 90);
            targetLabel.Location = new Point(31, 169);
            targetLabel.Size = new Size(589, 23);
            Controls.Add(targetLabel);

            launchCheckBox = new CheckBox();
            launchCheckBox.Text = "安装完成后启动 OBS";
            launchCheckBox.Checked = !options.NoLaunch;
            launchCheckBox.AutoSize = true;
            launchCheckBox.Location = new Point(31, 203);
            Controls.Add(launchCheckBox);

            statusLabel = new Label();
            statusLabel.Text = "请选择安装目录，然后点击“安装”。";
            statusLabel.AutoEllipsis = true;
            statusLabel.Location = new Point(31, 235);
            statusLabel.Size = new Size(420, 23);
            Controls.Add(statusLabel);

            progressBar = new ProgressBar();
            progressBar.Location = new Point(31, 263);
            progressBar.Size = new Size(420, 18);
            progressBar.Style = ProgressBarStyle.Blocks;
            Controls.Add(progressBar);

            installButton = new Button();
            installButton.Text = "安装";
            installButton.Location = new Point(466, 250);
            installButton.Size = new Size(74, 34);
            installButton.Click += InstallButtonClicked;
            Controls.Add(installButton);

            cancelButton = new Button();
            cancelButton.Text = "取消";
            cancelButton.Location = new Point(546, 250);
            cancelButton.Size = new Size(74, 34);
            cancelButton.Click += delegate { Close(); };
            Controls.Add(cancelButton);

            AcceptButton = installButton;
            CancelButton = cancelButton;
            FormClosing += InstallerFormClosing;
            UpdateTargetLabel();
        }

        private void UpdateTargetLabel()
        {
            try {
                string root = Path.GetFullPath(Environment.ExpandEnvironmentVariables(pathTextBox.Text));
                targetLabel.Text = "OBS 程序位置：" + Path.Combine(root, "obs-studio");
            }
            catch {
                targetLabel.Text = "OBS 程序位置：等待有效目录";
            }
        }

        private void BrowseButtonClicked(object sender, EventArgs eventArgs)
        {
            using (FolderBrowserDialog dialog = new FolderBrowserDialog()) {
                dialog.Description = "请选择 OBS 素材工作台的产品安装目录";
                dialog.ShowNewFolderButton = true;
                string candidate = FindExistingDirectory(pathTextBox.Text);
                if (!string.IsNullOrEmpty(candidate))
                    dialog.SelectedPath = candidate;
                if (dialog.ShowDialog(this) == DialogResult.OK)
                    pathTextBox.Text = dialog.SelectedPath;
            }
        }

        private static string FindExistingDirectory(string path)
        {
            try {
                string current = Path.GetFullPath(Environment.ExpandEnvironmentVariables(path));
                while (!string.IsNullOrEmpty(current) && !Directory.Exists(current))
                    current = Path.GetDirectoryName(current);
                return current;
            }
            catch {
                return string.Empty;
            }
        }

        private void InstallButtonClicked(object sender, EventArgs eventArgs)
        {
            string installRoot;
            try {
                installRoot = ValidateInstallRoot(pathTextBox.Text);
            }
            catch (Exception ex) {
                MessageBox.Show(
                    this,
                    ex.Message,
                    InstallerBootstrap.ProductName + "安装程序",
                    MessageBoxButtons.OK,
                    MessageBoxIcon.Warning);
                return;
            }

            string existingObs = Path.Combine(installRoot, "obs-studio", "bin", "64bit", "obs64.exe");
            if (File.Exists(existingObs)) {
                DialogResult overwrite = MessageBox.Show(
                    this,
                    "所选目录中已有 OBS 素材工作台。继续安装会先备份旧版本，再安装当前版本。",
                    "确认覆盖安装",
                    MessageBoxButtons.YesNo,
                    MessageBoxIcon.Question);
                if (overwrite != DialogResult.Yes)
                    return;
            }

            installing = true;
            SetControlsEnabled(false);
            statusLabel.Text = "正在校验并安装，请不要关闭窗口...";
            progressBar.Style = ProgressBarStyle.Marquee;
            progressBar.MarqueeAnimationSpeed = 25;

            InstallerOptions runOptions = options.Clone();
            runOptions.NoLaunch = !launchCheckBox.Checked;
            Task<InstallationResult> task = Task.Factory.StartNew(
                delegate { return RunInstallation(runOptions, installRoot); });
            task.ContinueWith(
                delegate(Task<InstallationResult> completed) {
                    InstallationFinished(installRoot, completed.Result);
                },
                TaskScheduler.FromCurrentSynchronizationContext());
        }

        private void InstallationFinished(string installRoot, InstallationResult result)
        {
            installing = false;
            progressBar.MarqueeAnimationSpeed = 0;
            progressBar.Style = ProgressBarStyle.Blocks;

            if (result.ExitCode == 0) {
                ExitCode = 0;
                statusLabel.Text = "安装完成。";
                progressBar.Value = 100;
                MessageBox.Show(
                    this,
                    "OBS 素材工作台安装完成。\n\n安装目录：" + installRoot,
                    "安装完成",
                    MessageBoxButtons.OK,
                    MessageBoxIcon.Information);
                Close();
                return;
            }

            ExitCode = result.ExitCode;
            SetControlsEnabled(true);
            statusLabel.Text = "安装失败，请检查错误信息后重试。";
            MessageBox.Show(
                this,
                LastUsefulLine(result.Output),
                "安装失败",
                MessageBoxButtons.OK,
                MessageBoxIcon.Error);
        }

        private static string LastUsefulLine(string output)
        {
            if (string.IsNullOrWhiteSpace(output))
                return "安装未完成，请查看安装日志。";

            string[] lines = output.Replace("\r", string.Empty).Split('\n');
            for (int index = lines.Length - 1; index >= 0; --index) {
                if (!string.IsNullOrWhiteSpace(lines[index]))
                    return lines[index].Trim();
            }
            return "安装未完成，请查看安装日志。";
        }

        private void SetControlsEnabled(bool enabled)
        {
            pathTextBox.Enabled = enabled;
            browseButton.Enabled = enabled;
            installButton.Enabled = enabled;
            cancelButton.Enabled = enabled;
            launchCheckBox.Enabled = enabled;
        }

        private void InstallerFormClosing(object sender, FormClosingEventArgs eventArgs)
        {
            if (!installing)
                return;

            eventArgs.Cancel = true;
            MessageBox.Show(
                this,
                "安装正在进行，请等待安装完成或回滚结束。",
                InstallerBootstrap.ProductName + "安装程序",
                MessageBoxButtons.OK,
                MessageBoxIcon.Information);
        }
    }
}

internal static class InstallerOptionsExtensions
{
    public static InstallerBootstrap.InstallerOptions Clone(this InstallerBootstrap.InstallerOptions source)
    {
        return new InstallerBootstrap.InstallerOptions {
            Unattended = source.Unattended,
            NoLaunch = source.NoLaunch,
            InstallRoot = source.InstallRoot,
            ShortcutRoot = source.ShortcutRoot,
            LaunchArguments = source.LaunchArguments,
            LogPath = source.LogPath
        };
    }
}
