namespace ProjectManager
{
    partial class ProjectManager
    {
        /// <summary>
        /// Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        /// Clean up any resources being used.
        /// </summary>
        /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Windows Form Designer generated code

        /// <summary>
        /// Required method for Designer support - do not modify
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(ProjectManager));
            this.buttonOpenProject = new System.Windows.Forms.Button();
            this.buttonSaveHeader = new System.Windows.Forms.Button();
            this.buttonSaveBinary = new System.Windows.Forms.Button();
            this.buttonPlaySong = new System.Windows.Forms.Button();
            this.buttonExportWav = new System.Windows.Forms.Button();
            this.textBoxOutput = new System.Windows.Forms.TextBox();
            this.buttonProjectDetails = new System.Windows.Forms.Button();
            this.buttonExportRenoise = new System.Windows.Forms.Button();
            this.btnCopyCPP = new System.Windows.Forms.Button();
            this.cbxBoundsMode = new System.Windows.Forms.ComboBox();
            this.label2 = new System.Windows.Forms.Label();
            this.cbxTimestampScale = new System.Windows.Forms.ComboBox();
            this.label3 = new System.Windows.Forms.Label();
            this.label4 = new System.Windows.Forms.Label();
            this.cbxDurationScale = new System.Windows.Forms.ComboBox();
            this.SuspendLayout();
            // 
            // buttonOpenProject
            // 
            this.buttonOpenProject.Location = new System.Drawing.Point(12, 12);
            this.buttonOpenProject.Name = "buttonOpenProject";
            this.buttonOpenProject.Size = new System.Drawing.Size(104, 23);
            this.buttonOpenProject.TabIndex = 0;
            this.buttonOpenProject.Text = "Open Project";
            this.buttonOpenProject.UseVisualStyleBackColor = true;
            this.buttonOpenProject.Click += new System.EventHandler(this.buttonOpenProject_Click);
            // 
            // buttonSaveHeader
            // 
            this.buttonSaveHeader.Enabled = false;
            this.buttonSaveHeader.Location = new System.Drawing.Point(12, 138);
            this.buttonSaveHeader.Name = "buttonSaveHeader";
            this.buttonSaveHeader.Size = new System.Drawing.Size(104, 23);
            this.buttonSaveHeader.TabIndex = 0;
            this.buttonSaveHeader.Text = "Save C++ Header";
            this.buttonSaveHeader.UseVisualStyleBackColor = true;
            this.buttonSaveHeader.Click += new System.EventHandler(this.buttonSaveHeader_Click);
            // 
            // buttonSaveBinary
            // 
            this.buttonSaveBinary.Enabled = false;
            this.buttonSaveBinary.Location = new System.Drawing.Point(11, 196);
            this.buttonSaveBinary.Name = "buttonSaveBinary";
            this.buttonSaveBinary.Size = new System.Drawing.Size(104, 23);
            this.buttonSaveBinary.TabIndex = 0;
            this.buttonSaveBinary.Text = "Save Binary Song";
            this.buttonSaveBinary.UseVisualStyleBackColor = true;
            this.buttonSaveBinary.Click += new System.EventHandler(this.buttonSaveBinary_Click);
            // 
            // buttonPlaySong
            // 
            this.buttonPlaySong.Enabled = false;
            this.buttonPlaySong.Location = new System.Drawing.Point(12, 90);
            this.buttonPlaySong.Name = "buttonPlaySong";
            this.buttonPlaySong.Size = new System.Drawing.Size(104, 23);
            this.buttonPlaySong.TabIndex = 0;
            this.buttonPlaySong.Text = "Play Song";
            this.buttonPlaySong.UseVisualStyleBackColor = true;
            this.buttonPlaySong.Click += new System.EventHandler(this.buttonPlaySong_Click);
            // 
            // buttonExportWav
            // 
            this.buttonExportWav.Enabled = false;
            this.buttonExportWav.Location = new System.Drawing.Point(11, 225);
            this.buttonExportWav.Name = "buttonExportWav";
            this.buttonExportWav.Size = new System.Drawing.Size(104, 23);
            this.buttonExportWav.TabIndex = 0;
            this.buttonExportWav.Text = "Export Wav";
            this.buttonExportWav.UseVisualStyleBackColor = true;
            this.buttonExportWav.Click += new System.EventHandler(this.buttonExportWav_Click);
            // 
            // textBoxOutput
            // 
            this.textBoxOutput.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.textBoxOutput.Location = new System.Drawing.Point(147, 12);
            this.textBoxOutput.Multiline = true;
            this.textBoxOutput.Name = "textBoxOutput";
            this.textBoxOutput.ScrollBars = System.Windows.Forms.ScrollBars.Vertical;
            this.textBoxOutput.Size = new System.Drawing.Size(607, 425);
            this.textBoxOutput.TabIndex = 2;
            // 
            // buttonProjectDetails
            // 
            this.buttonProjectDetails.Enabled = false;
            this.buttonProjectDetails.Location = new System.Drawing.Point(11, 41);
            this.buttonProjectDetails.Name = "buttonProjectDetails";
            this.buttonProjectDetails.Size = new System.Drawing.Size(104, 23);
            this.buttonProjectDetails.TabIndex = 4;
            this.buttonProjectDetails.Text = "Project Details";
            this.buttonProjectDetails.UseVisualStyleBackColor = true;
            this.buttonProjectDetails.Click += new System.EventHandler(this.buttonProjectDetails_Click);
            // 
            // buttonExportRenoise
            // 
            this.buttonExportRenoise.Enabled = false;
            this.buttonExportRenoise.Location = new System.Drawing.Point(11, 401);
            this.buttonExportRenoise.Name = "buttonExportRenoise";
            this.buttonExportRenoise.Size = new System.Drawing.Size(104, 36);
            this.buttonExportRenoise.TabIndex = 0;
            this.buttonExportRenoise.Text = "Export Renoise Patch Song";
            this.buttonExportRenoise.UseVisualStyleBackColor = true;
            this.buttonExportRenoise.Click += new System.EventHandler(this.buttonExportRenoise_Click);
            // 
            // btnCopyCPP
            // 
            this.btnCopyCPP.Location = new System.Drawing.Point(11, 167);
            this.btnCopyCPP.Name = "btnCopyCPP";
            this.btnCopyCPP.Size = new System.Drawing.Size(104, 23);
            this.btnCopyCPP.TabIndex = 6;
            this.btnCopyCPP.Text = "Copy .hpp";
            this.btnCopyCPP.UseVisualStyleBackColor = true;
            this.btnCopyCPP.Click += new System.EventHandler(this.btnCopyCPP_Click);
            // 
            // cbxBoundsMode
            // 
            this.cbxBoundsMode.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.cbxBoundsMode.FormattingEnabled = true;
            this.cbxBoundsMode.Location = new System.Drawing.Point(11, 285);
            this.cbxBoundsMode.Name = "cbxBoundsMode";
            this.cbxBoundsMode.Size = new System.Drawing.Size(104, 21);
            this.cbxBoundsMode.TabIndex = 7;
            // 
            // label2
            // 
            this.label2.AutoSize = true;
            this.label2.Location = new System.Drawing.Point(12, 269);
            this.label2.Name = "label2";
            this.label2.Size = new System.Drawing.Size(72, 13);
            this.label2.TabIndex = 8;
            this.label2.Text = "Bounds mode";
            // 
            // cbxTimestampScale
            // 
            this.cbxTimestampScale.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.cbxTimestampScale.FormattingEnabled = true;
            this.cbxTimestampScale.Items.AddRange(new object[] {
            "1",
            "2",
            "4",
            "8",
            "16",
            "32",
            "64",
            "128",
            "256",
            "512"});
            this.cbxTimestampScale.Location = new System.Drawing.Point(12, 325);
            this.cbxTimestampScale.Name = "cbxTimestampScale";
            this.cbxTimestampScale.Size = new System.Drawing.Size(104, 21);
            this.cbxTimestampScale.TabIndex = 9;
            // 
            // label3
            // 
            this.label3.AutoSize = true;
            this.label3.Location = new System.Drawing.Point(12, 309);
            this.label3.Name = "label3";
            this.label3.Size = new System.Drawing.Size(117, 13);
            this.label3.TabIndex = 10;
            this.label3.Text = "EventQuantizeSamples";
            // 
            // label4
            // 
            this.label4.AutoSize = true;
            this.label4.Location = new System.Drawing.Point(12, 349);
            this.label4.Name = "label4";
            this.label4.Size = new System.Drawing.Size(129, 13);
            this.label4.TabIndex = 12;
            this.label4.Text = "DurationQuantizeSamples";
            // 
            // cbxDurationScale
            // 
            this.cbxDurationScale.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.cbxDurationScale.FormattingEnabled = true;
            this.cbxDurationScale.Items.AddRange(new object[] {
            "1",
            "2",
            "4",
            "8",
            "16",
            "32",
            "64",
            "128",
            "256",
            "512"});
            this.cbxDurationScale.Location = new System.Drawing.Point(12, 365);
            this.cbxDurationScale.Name = "cbxDurationScale";
            this.cbxDurationScale.Size = new System.Drawing.Size(104, 21);
            this.cbxDurationScale.TabIndex = 11;
            // 
            // ProjectManager
            // 
            this.AllowDrop = true;
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(766, 449);
            this.Controls.Add(this.label4);
            this.Controls.Add(this.cbxDurationScale);
            this.Controls.Add(this.label3);
            this.Controls.Add(this.cbxTimestampScale);
            this.Controls.Add(this.label2);
            this.Controls.Add(this.cbxBoundsMode);
            this.Controls.Add(this.btnCopyCPP);
            this.Controls.Add(this.buttonProjectDetails);
            this.Controls.Add(this.textBoxOutput);
            this.Controls.Add(this.buttonExportRenoise);
            this.Controls.Add(this.buttonExportWav);
            this.Controls.Add(this.buttonPlaySong);
            this.Controls.Add(this.buttonSaveBinary);
            this.Controls.Add(this.buttonSaveHeader);
            this.Controls.Add(this.buttonOpenProject);
            this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
            this.Name = "ProjectManager";
            this.Text = "Project Manager";
            this.DragDrop += new System.Windows.Forms.DragEventHandler(this.ProjectManager_DragDrop);
            this.DragEnter += new System.Windows.Forms.DragEventHandler(this.ProjectManager_DragEnter);
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.Button buttonOpenProject;
        private System.Windows.Forms.Button buttonSaveHeader;
        private System.Windows.Forms.Button buttonSaveBinary;
        private System.Windows.Forms.Button buttonPlaySong;
        private System.Windows.Forms.Button buttonExportWav;
        private System.Windows.Forms.TextBox textBoxOutput;
        private System.Windows.Forms.Button buttonProjectDetails;
        private System.Windows.Forms.Button buttonExportRenoise;
        private System.Windows.Forms.Button btnCopyCPP;
        private System.Windows.Forms.ComboBox cbxBoundsMode;
        private System.Windows.Forms.Label label2;
        private System.Windows.Forms.ComboBox cbxTimestampScale;
        private System.Windows.Forms.Label label3;
        private System.Windows.Forms.Label label4;
        private System.Windows.Forms.ComboBox cbxDurationScale;
    }
}

