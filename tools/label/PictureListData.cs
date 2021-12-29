using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace myy_label_cs
{
    class PictureListData
    {
        public PictureListData(FileInfo fileInfo)
        {
            info = fileInfo;
            string gtfile = fileInfo.FullName + ".gt.txt";
            labeled = File.Exists(gtfile);
        }
        public FileInfo info;
        public bool labeled;
    }
}
