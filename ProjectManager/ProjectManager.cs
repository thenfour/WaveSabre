using System;
using System.Diagnostics;
using System.IO;
using System.Windows.Forms;
using WaveSabreConvert;
using static System.Windows.Forms.VisualStyles.VisualStyleElement;

namespace ProjectManager
{
    public partial class ProjectManager : Form
    {
        private Song song;
        private string fileName;
        private EventLogger logger;

        public ProjectManager()
        {
            InitializeComponent();

            cbxBoundsMode.DataSource = Enum.GetValues(typeof(BoundsMode));
            cbxBoundsMode.DisplayMember = "ToString";
            cbxBoundsMode.SelectedIndex = 0;
        }

        private void buttonOpenProject_Click(object sender, EventArgs e)
        {
            var ofd = new OpenFileDialog();
            ofd.Filter = "Project Files|*.als;*.flp;*.xrns;*.rpp";
            if (ofd.ShowDialog() == DialogResult.OK)
            {
                OpenProject(ofd.FileName);
            }
        }

        private void OpenProject(string projectFile)
        {
            //try
            //{
                textBoxOutput.Text = "";
                logger = new EventLogger();
                logger.OnLog += OnLogEvent;

                song = new ProjectConverter().Convert(projectFile, logger, new ConvertOptions { mBoundsMode = (BoundsMode)cbxBoundsMode.SelectedItem });
                fileName = Path.GetFileNameWithoutExtension(projectFile);
                logger.WriteLine("Done.");
                Enable();
            //}
            //catch (Exception err)
            //{
            //    MessageBox.Show(err.Message);
            //}
            
        }

        private void OnLogEvent(object sender, EventArgs e)
        {
            var data = (LogEvent)e;
            textBoxOutput.AppendText(data.Output);
        }

        private void Enable()
        {
            buttonSaveHeader.Enabled = true;
            buttonSaveBinary.Enabled = true;
            buttonPlaySong.Enabled = true;
            buttonExportWav.Enabled = true;
            buttonProjectDetails.Enabled = true;
            buttonExportRenoise.Enabled = true;
        }

        private void buttonSaveHeader_Click(object sender, EventArgs e)
        {
            try
            {
                var header = new Serializer().Serialize(song);
                var sfd = new SaveFileDialog();
                sfd.Filter = "C++ Header|*.h";
                if (sfd.ShowDialog() == DialogResult.OK)
                {
                    File.WriteAllText(sfd.FileName, header);
                }
            }
            catch (Exception err)
            {
                MessageBox.Show(err.Message);
            }
        }

        private void buttonSaveBinary_Click(object sender, EventArgs e)
        {
            try
            {
                var bin = new Serializer().SerializeBinary(song);
                var sfd = new SaveFileDialog();
                sfd.Filter = "Binary Song File|*.bin";
                sfd.FileName = fileName;
                if (sfd.ShowDialog() == DialogResult.OK)
                {
                    File.WriteAllBytes(sfd.FileName, bin.CompleteSong.GetByteArray());
                }
            }
            catch (Exception err)
            {
                MessageBox.Show(err.Message);
            }
        }

        private void buttonPlaySong_Click(object sender, EventArgs e)
        {
            try
            {
                var bin = new Serializer().SerializeBinary(song);
                var tempFile = Path.GetTempPath() + "WaveSabre.bin";
                GetRid(tempFile);
                File.WriteAllBytes(tempFile, bin.CompleteSong.GetByteArray());
                logger.WriteLine($@"WaveSabreStandAlonePlayer.exe {tempFile}");
                var proc = Process.Start(@"WaveSabreStandAlonePlayer.exe", string.Format("\"{0}\"", tempFile));
                proc.WaitForExit();
                //GetRid(tempFile); // for the sake of demoscene dev, it's not so useful to remove this file.
            }
            catch (Exception err)
            {
                MessageBox.Show(err.Message);
            }
        }

        private void GetRid(string killFile)
        {
            if (File.Exists(killFile))
            {
                File.Delete(killFile);
            }
        }

        private void buttonExportWav_Click(object sender, EventArgs e)
        {
            try
            {
                var sfd = new SaveFileDialog();
                sfd.Filter = "Wav file|*.wav";
                sfd.FileName = fileName;
                if (sfd.ShowDialog() == DialogResult.OK)
                {
                    var bin = new Serializer().SerializeBinary(song);
                    var tempFile = Path.GetTempPath() + "WaveSabre.bin";
                    GetRid(tempFile);
                    File.WriteAllBytes(tempFile, bin.CompleteSong.GetByteArray());
                    var proc = Process.Start(@"WaveSabreStandAlonePlayer.exe", string.Format("\"{0}\" -w \"{1}\"", tempFile, sfd.FileName));
                    proc.WaitForExit();
                    GetRid(tempFile);
                }
            }
            catch (Exception err)
            {
                MessageBox.Show(err.Message);
            }
        }

        private void buttonProjectDetails_Click(object sender, EventArgs e)
        {
            var details = new ProjectDetails(song);
            details.ShowDialog();
        }

        private void buttonExportRenoise_Click(object sender, EventArgs e)
        {
            try
            {
                var inject = new RenoiseInject();
                var rnsSong = inject.InjectPatches(song, logger);

                var sfd = new SaveFileDialog();
                sfd.Filter = "Renoise Song File|*.xrns";
                sfd.FileName = fileName;
                if (sfd.ShowDialog() == DialogResult.OK)
                {
                    var rnsParser = new RenoiseParser();
                    rnsParser.Save(rnsSong, sfd.FileName);
                }
            }
            catch (Exception err)
            {
                MessageBox.Show(err.Message);
            }
        }

        private void btnCopyCPP_Click(object sender, EventArgs e)
        {
            try
            {
                var header = new Serializer().Serialize(song);
                Clipboard.SetText(header);
                MessageBox.Show("Copied!");
            }
            catch (Exception err)
            {
                MessageBox.Show(err.Message);
            }
        }

    }
}
