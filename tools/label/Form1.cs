using OpenCvSharp;
using OpenCvSharp.Extensions;
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace myy_label_cs
{
    public partial class Form1 : Form
    {
        PictureListData[] ls;

        List<Ellipse> ells;

        Mat pic;

        Bitmap bitmap, bimg;
        Graphics g;

        int cur_file;

        int curEll;

        double zoom;

        int move_x, move_y;

        bool labeling, draging, shift;

        int drag_start_x, drag_start_y, drag_start_move_x, drag_start_move_y;

        int gc_cnt;

        public Form1()
        {
            InitializeComponent();
            move_x = move_y = 0;
            zoom = 1;
            labeling = false;
            shift = false;
            draging = false;
            MouseWheel += form1_MouseWheel;
            gc_cnt = 0;
            ells = new List<Ellipse>();
            Resize += do_resize;
            imp_do_resize();
            cur_file = -1;
        }

        private void button1_Click(object sender, EventArgs e)
        {
            Enabled = false;
            Text = "Waiting...";
            FolderBrowserDialog fileDialog = new FolderBrowserDialog();
            if(fileDialog.ShowDialog()==DialogResult.OK)
            {
                textBox1.Text = fileDialog.SelectedPath;
                Bitmap bitmap = new Bitmap(pictureBox1.Width, pictureBox1.Height);
                Graphics g = Graphics.FromImage(bitmap);
                g.Clear(Color.White);
                pictureBox1.Image = bitmap;

                string path = textBox1.Text;

                DirectoryInfo root = new DirectoryInfo(path);
                String[] exts = { "*.jpg", "*.png", "*.bmp", "*.jpeg" };
                FileInfo[] files=new FileInfo[0];
                foreach (String ext in exts)
                {
                    FileInfo[] filesi = root.GetFiles(ext);
                    files=files.Concat(filesi).ToArray();
                }
                ls = new PictureListData[files.Length];
                for (int i = 0; i < files.Length; i++)
                {
                    ls[i] = new PictureListData(files[i]);
                }
                listView1.Items.Clear();
                listView1.BeginUpdate();
                foreach (var i in ls)
                {
                    ListViewItem item = new ListViewItem(i.info.Name);
                    item.SubItems.Add(i.labeled ? "Y" : "N");
                    item.Name = i.info.Name;
                    listView1.Items.Add(item);
                }
                listView1.EndUpdate();
                labeling = false;
                cur_file = -1;
            }

            
            Text = "Load";
            Enabled = true;
        }

        public void LoadEllipses(string fname)
        {
            string gt = fname + ".gt.txt";

            StreamReader reader = new StreamReader(gt);
            int n = int.Parse(reader.ReadLine());
            for(int i=0;i<n;i++)
            {
                string[] p = reader.ReadLine().Split(' ');
                Ellipse ell = new Ellipse(float.Parse(p[0]), float.Parse(p[1]), float.Parse(p[2]), float.Parse(p[3]), float.Parse(p[4]));
                ells.Add(ell);
            }
            reader.Close();
        }

        void do_save()
        {
            int n = 0;
            foreach (var ell in ells)
            {
                if (double.IsNaN(ell.ell.Center.X))
                    continue;
                n++;
            }
            if (n == 0 )
            {
                if(File.Exists(ls[cur_file].info.FullName + ".gt.txt"))
                    File.Delete(ls[cur_file].info.FullName + ".gt.txt");
                ls[cur_file].labeled = false;
                listView1.Items[ls[cur_file].info.Name].SubItems[1].Text = "N";
                return;
            }
            try
            {
                StreamWriter sw = new StreamWriter(ls[cur_file].info.FullName + ".gt.txt", false);
                sw.WriteLine(n.ToString());
                foreach (var ell in ells)
                {
                    if (double.IsNaN(ell.ell.Center.X))
                        continue;
                    sw.WriteLine(ell.ToString());
                }
                sw.Close();
                ls[cur_file].labeled = true;
                listView1.Items[ls[cur_file].info.Name].SubItems[1].Text = "Y";
            }
            catch(Exception e)
            {
                MessageBox.Show("标注文件保存失败：" + ls[cur_file].info.FullName);
            }
        }

        private void listView1_KeyDown(object sender, KeyEventArgs e)
        {
            if (e.KeyCode == Keys.ShiftKey)
            {
                shift = true;
            }
            else if (e.KeyCode == Keys.Space)
            {
                curEll = -1;
                draw_image(true);
            }
            else if (e.KeyCode == Keys.Z)
            {
                zoom = Math.Min(5, zoom + 0.1);
                draw_image(false);
            }
            else if (e.KeyCode == Keys.C)
            {
                zoom = Math.Max(0.1, zoom - 0.1);
                draw_image(false);
            }
            else if (e.KeyCode == Keys.W)
            {
                move_y += 10;
                draw_image(false);
            }
            else if (e.KeyCode == Keys.S)
            {
                if(e.Control)
                {
                    do_save();
                    return;
                }
                move_y -= 10;
                draw_image(false);
            }
            else if (e.KeyCode == Keys.A)
            {
                move_x += 10;
                draw_image(false);
            }
            else if (e.KeyCode == Keys.D)
            {
                move_x -= 10;
                draw_image(false);
            }
            else if (e.KeyCode == Keys.X)
            {
                if (pic.Width * pictureBox1.Height < pictureBox1.Width * pic.Height)
                {
                    zoom = (double)pictureBox1.Height / (double)pic.Height;
                    move_x = (int)((pictureBox1.Width - zoom * pic.Width) / 2);
                    move_y = 0;
                }
                else
                {
                    zoom = (double)pictureBox1.Width / (double)pic.Width;
                    move_x = 0;
                    move_y = (int)((pictureBox1.Height - zoom * pic.Height) / 2);
                }
                draw_image(false);
            }
            else if (e.KeyCode == Keys.Delete)
            {
                if(curEll!=-1)
                {
                    ells.RemoveAt(curEll);
                    curEll = ells.Count - 1;
                    draw_image(true);
                }
            }
        }

        private void do_resize(object sender, EventArgs e)
        {
            imp_do_resize();
        }

        private void imp_do_resize()
        {
            int margin = 12;
            pictureBox1.Height = ClientSize.Height - margin * 2;
            button1.Location = new System.Drawing.Point(ClientSize.Width - button1.Size.Width - margin, margin);
            textBox1.Location = new System.Drawing.Point(ClientSize.Width - textBox1.Size.Width - margin,
                button1.Size.Height + margin * 2);
            listView1.Location = new System.Drawing.Point(ClientSize.Width - listView1.Size.Width - margin,
                button1.Size.Height + textBox1.Size.Height + margin * 3);
            listView1.Height = ClientSize.Height - listView1.Location.Y - margin;
            label1.Location = new System.Drawing.Point(ClientSize.Width - textBox1.Size.Width - margin, margin);
            pictureBox1.Width = textBox1.Location.X - margin * 2;
            bitmap = new Bitmap(pictureBox1.Width, pictureBox1.Height);
            g = Graphics.FromImage(bitmap);
            draw_image(false);
        }

        private void listView1_KeyUp(object sender, KeyEventArgs e)
        {
            if (e.KeyCode == Keys.ShiftKey)
            {
                shift = false;
            }
        }

        private void draw_image(bool redraw)
        {
            if(redraw)
            {
                Mat img = pic.Clone();
                for(int i=0;i<ells.Count;i++)
                {
                    if(i==curEll)
                    {
                        Cv2.Ellipse(img, ells[i].ell, new Scalar(0, 255, 0), 1);
                        if (ells[i].pts != null)
                        {
                            for (int j = 0; j < ells[i].pts.Count; j++)
                            {
                                Cv2.Circle(img, ells[i].pts[j], 1, new Scalar(0, 0, 0), -1);
                            }
                        }
                    }
                    else
                    {
                        Cv2.Ellipse(img, ells[i].ell, new Scalar(0, 0, 255), 1);
                    }
                }
                bimg = img.ToBitmap();
                img.Dispose();
                gc_cnt++;
                if(gc_cnt==5)
                {
                    GC.Collect();
                    gc_cnt = 0;
                }
            }

            g.Clear(Color.White);
            if(bimg!=null)
                g.DrawImage(bimg, new Rectangle(move_x, move_y, (int)(pic.Width * zoom), (int)(pic.Height * zoom)));
            pictureBox1.Image = bitmap;
            
        }

        private void pictureBox1_MouseMove(object sender, MouseEventArgs e)
        {
            if(e.Button == MouseButtons.Middle)
            {
                int dx = e.X - drag_start_x, dy = e.Y - drag_start_y;
                move_x = drag_start_move_x + (int)(dx );
                move_y = drag_start_move_y + (int)(dy );
                draw_image(false);
            }
        }

        private void pictureBox1_MouseUp(object sender, MouseEventArgs e)
        {
            if (e.Button == MouseButtons.Middle)
            {
                draging = false;
                draw_image(false);
            }
        }

        private void listView1_ItemActivate(object sender, EventArgs e)
        {
            labeling = true;
            int sel = listView1.SelectedIndices[0];
            if(cur_file!=-1)
            {
                do_save();
            }
            cur_file = sel;
            pic = Cv2.ImRead(ls[sel].info.FullName);
            ells.Clear();
            if(ls[sel].labeled)
            {
                LoadEllipses(ls[sel].info.FullName);
            }
            
            curEll = -1;
            if(pic.Width* pictureBox1.Height <pictureBox1.Width* pic.Height )
            {
                zoom = (double)pictureBox1.Height / pic.Height;
                move_x = (int)((pictureBox1.Width - zoom * pic.Width) / 2);
                move_y = 0;
            }
            else
            {
                zoom = (double)pictureBox1.Width / pic.Width;
                move_x = 0;
                move_y = (int)((pictureBox1.Height - zoom * pic.Height) / 2);
            }

            draw_image(true);
        }

        private void pictureBox1_MouseDown(object sender, MouseEventArgs e)
        {
            if (!labeling)
                return;
            double x = (e.X - move_x) / zoom, y = (e.Y - move_y) / zoom;
            if (e.Button==MouseButtons.Left)
            {
                if (shift)
                {
                    Mat tmp = new Mat<Vec3b>(pic.Size(),new Scalar(0,0,0));
                    for(int i=0;i<ells.Count;i++)
                    {
                        Cv2.Ellipse(tmp, ells[i].ell, new Scalar(0, 0, i+1), 7);
                    }
                    int sel = tmp.At<Vec3b>((int)y, (int)x).Item2;
                    if(sel!=-1)
                    {
                        curEll = sel-1;
                    }
                }
                else
                {
                    if (curEll < 0)
                    {
                        curEll = ells.Count;
                        ells.Add(new Ellipse());
                    }
                    Ellipse ell = ells[curEll];
                    if (ell.pts != null)
                    {
                        ell.pts.Add(new OpenCvSharp.Point(x, y));
                        ell.ComputeEllipse();
                    }
                }
            }
            else if(e.Button==MouseButtons.Middle)
            {
                draging = true;
                drag_start_x = e.X;
                drag_start_y = e.Y;
                drag_start_move_x = move_x;
                drag_start_move_y = move_y;
            }
            else if (e.Button == MouseButtons.Right)
            {
                if (curEll < 0 || ells[curEll].pts==null)
                    return;

                int no = -1;
                double mdis = 0;
                for(int i=0;i<ells[curEll].pts.Count;i++)
                {
                    double dis = ells[curEll].pts[i].DistanceTo(new OpenCvSharp.Point(x, y));
                    if(dis<5)
                    {
                        if(no==-1||mdis>dis)
                        {
                            no = i;
                            mdis = dis;
                        }
                    }
                }
                if(no!=-1)
                {
                    ells[curEll].pts.RemoveAt(no);
                    ells[curEll].ComputeEllipse();
                }
            }
            draw_image(true);
        }

        private void form1_MouseWheel(object sender, MouseEventArgs e)
        {
            double x = (e.X - move_x) / zoom, y = (e.Y - move_y) / zoom;
            if (!labeling)
                return;
            if (e.Delta > 0)
            {
                zoom = Math.Max(0.1, zoom - 0.1);
                move_x = e.X - (int)(x * zoom);
                move_y = e.Y - (int)(y * zoom);
            }
            else if (e.Delta < 0)
            {
                zoom = Math.Min(10, zoom + 0.1);
                move_x = e.X - (int)(x * zoom);
                move_y = e.Y - (int)(y * zoom);
            }
            draw_image(false);
        }
    }
}
