using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices.WindowsRuntime;
using System.Threading.Tasks;
using Windows.Foundation;
using Windows.Foundation.Collections;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Controls.Primitives;
using Windows.UI.Xaml.Data;
using Windows.UI.Xaml.Input;
using Windows.UI.Xaml.Media;
using Windows.UI.Xaml.Media.Imaging;
using Windows.UI.Xaml.Navigation;
using Windows.Storage.Pickers;
using Windows.Storage;
using Windows.Storage.Streams;

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

            this.NavigationCacheMode = NavigationCacheMode.Required;
        }

        /// <summary>
        /// Invoked when this page is about to be displayed in a Frame.
        /// </summary>
        /// <param name="e">Event data that describes how this page was reached.
        /// This parameter is typically used to configure the page.</param>
        protected override void OnNavigatedTo(NavigationEventArgs e)
        {
            // TODO: Prepare page for display here.

            // TODO: If your application contains multiple pages, ensure that you are
            // handling the hardware Back button by registering for the
            // Windows.Phone.UI.Input.HardwareButtons.BackPressed event.
            // If you are using the NavigationHelper provided by some templates,
            // this event is handled for you.
        }

		internal async void ContinuePick(StorageFile file)
		{
			Lumia.Imaging.BitmapRenderer renderer = new Lumia.Imaging.BitmapRenderer();
			renderer.ColorMode = Lumia.Imaging.ColorMode.Yuv420P;
			renderer.Source = new Lumia.Imaging.StorageFileImageSource(file);
			var bmp = await renderer.RenderAsync();
			var surfaceImageSource = new SurfaceImageSource((int)bmp.Dimensions.Width, (int)bmp.Dimensions.Height);
			var yuvRenderer = YuvRenderer.YuvD3DRenderer.CreateForSwapChainPanel(root, (uint)bmp.Dimensions.Width, (uint)bmp.Dimensions.Height, YuvRenderer.StretchMode.Uniform);
			Task.Run(() =>
			{
				// convert to nv12.
				var yReader = DataReader.FromBuffer(bmp.Buffers[0].Buffer);
				var uReader = DataReader.FromBuffer(bmp.Buffers[1].Buffer);
				var vReader = DataReader.FromBuffer(bmp.Buffers[2].Buffer);
				byte[] yData = new byte[bmp.Buffers[0].Buffer.Length];
				byte[] uData = new byte[bmp.Buffers[1].Buffer.Length];
				byte[] vData = new byte[bmp.Buffers[2].Buffer.Length];
				yReader.ReadBytes(yData);
				uReader.ReadBytes(uData);
				vReader.ReadBytes(vData);
				var height = (int)(bmp.Dimensions.Height);
				var halfWidth = ((int)(bmp.Dimensions.Width)) / 2;
				var halfHeight = height / 2;
                var heightAndHalf = height + halfHeight;
                var nv12Data = new byte[bmp.Buffers[0].Pitch * heightAndHalf];

				Array.Copy(yData, nv12Data, yData.Length);
				for (int y = 0; y < halfHeight; ++y)
				{
					var firstPixelOfRow = yData.Length + y * bmp.Buffers[0].Pitch;
                    for (int x = 0; x < halfWidth; ++x)
					{
						nv12Data[x * 2 + firstPixelOfRow] = uData[y * bmp.Buffers[1].Pitch + x];
						nv12Data[x * 2 + firstPixelOfRow +1] = vData[y * bmp.Buffers[2].Pitch + x];
					}
				}

				while (true)
				{
					yuvRenderer.RenderNV12(nv12Data, bmp.Buffers[0].Pitch);
				}
			});
		}

		private async void button_Click(object sender, RoutedEventArgs e)
		{
			var picker = new Windows.Storage.Pickers.FileOpenPicker();
			picker.FileTypeFilter.Add(".jpg");
			picker.PickSingleFileAndContinue();
			
		}

		private void clearbutton_Click(object sender, RoutedEventArgs e)
		{

		}
	}
}
