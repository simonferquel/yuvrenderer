using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices.WindowsRuntime;
using System.Threading.Tasks;
using Windows.Foundation;
using Windows.Foundation.Collections;
using Windows.Storage.Streams;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Controls.Primitives;
using Windows.UI.Xaml.Data;
using Windows.UI.Xaml.Input;
using Windows.UI.Xaml.Media;
using Windows.UI.Xaml.Media.Imaging;
using Windows.UI.Xaml.Navigation;

// The Blank Page item template is documented at http://go.microsoft.com/fwlink/?LinkId=234238

namespace SampleApp
{
    /// <summary>
    /// An empty page that can be used on its own or navigated to within a Frame.
    /// </summary>
    public sealed partial class MainPage : Page
    {
        public MainPage()
        {
            this.InitializeComponent();
        }

        private async void button_Click(object sender, RoutedEventArgs e)
        {
            var picker = new Windows.Storage.Pickers.FileOpenPicker();
            picker.FileTypeFilter.Add(".jpg");
            var file = await picker.PickSingleFileAsync();
            Lumia.Imaging.BitmapRenderer renderer = new Lumia.Imaging.BitmapRenderer();
            renderer.ColorMode = Lumia.Imaging.ColorMode.Yuv420P;
            renderer.Source = new Lumia.Imaging.StorageFileImageSource(file);
            var bmp = await renderer.RenderAsync();
            var surfaceImageSource = new SurfaceImageSource((int)bmp.Dimensions.Width, (int)bmp.Dimensions.Height);
            var yuvRenderer = YuvRenderer.YuvD3DRenderer.CreateForSwapChainPanel(root, (uint)bmp.Dimensions.Width, (uint)bmp.Dimensions.Height, YuvRenderer.StretchMode.UniformToFill);
			Task.Run(() =>
			{
				//// convert to nv12.
				//var yReader = DataReader.FromBuffer(bmp.Buffers[0].Buffer);
				//var uReader = DataReader.FromBuffer(bmp.Buffers[1].Buffer);
				//var vReader = DataReader.FromBuffer(bmp.Buffers[2].Buffer);
				//byte[] yData = new byte[bmp.Buffers[0].Buffer.Length];
				//byte[] uData = new byte[bmp.Buffers[1].Buffer.Length];
				//byte[] vData = new byte[bmp.Buffers[2].Buffer.Length];
				//yReader.ReadBytes(yData);
				//uReader.ReadBytes(uData);
				//vReader.ReadBytes(vData);
				//var height = (int)(bmp.Dimensions.Height);
				//var halfWidth = ((int)(bmp.Dimensions.Width)) / 2;
				//var halfHeight = height / 2;
				//var heightAndHalf = height + halfHeight;
				//var nv12Data = new byte[bmp.Buffers[0].Pitch * heightAndHalf];

				//Array.Copy(yData, nv12Data, yData.Length);
				//Parallel.For(0, halfHeight, y =>
				//{
				//	var firstPixelOfRow = yData.Length + y * bmp.Buffers[0].Pitch;
				//	for (int x = 0; x < halfWidth; ++x)
				//	{
				//		nv12Data[x * 2 + firstPixelOfRow] = uData[y * bmp.Buffers[1].Pitch + x];
				//		nv12Data[x * 2 + firstPixelOfRow + 1] = vData[y * bmp.Buffers[2].Pitch + x];
				//	}
				//});

				while (true)
				{
					yuvRenderer.RenderYuv420P(bmp.Buffers[0].Buffer, bmp.Buffers[1].Buffer, bmp.Buffers[2].Buffer, bmp.Buffers[0].Pitch, bmp.Buffers[1].Pitch);
				}
			});
		}
    }
}
