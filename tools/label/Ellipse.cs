using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using OpenCvSharp;

namespace myy_label_cs
{
	public class Ellipse
	{
		public Ellipse(float x, float y, float a, float b, float theta)
        {
			ell = new RotatedRect(new Point2f(x, y), new Size2f(a*2,b*2), theta);
        }

		public Ellipse()
        {
			pts = new List<Point>();
        }

		public List<Point> pts;

		public RotatedRect ell;

		public void ComputeEllipse()
		{
			if (pts == null)
				return;
			if(pts.Count<5)
			{
				Point2f center=new Point2f(0,0);
				for (int i = 0; i < pts.Count; i++)
					center += pts[i];
				center*=1.0/(pts.Count);
				double max_dis = 20;
				for(int i=0;i<pts.Count; i++)
					max_dis=Math.Max(max_dis, center.DistanceTo(pts[i]));
				max_dis*=2;
				ell = new RotatedRect(center, new Size2f(max_dis, max_dis),0);
			}
			else
			{
				ell = Cv2.FitEllipse(pts);
			}
		}

		override public string ToString() 
		{
			string ss = ell.Center.X.ToString("f4")+" "
				+ ell.Center.Y.ToString("f4") + " "
				+ (ell.Size.Width/2).ToString("f4") + " "
				+ (ell.Size.Height/2).ToString("f4") + " "
				+ ell.Angle.ToString("f4");

			return ss;
		}
	};
}
