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
            var yuvRenderer = YuvRenderer.YuvD3DRenderer.CreateForSwapChainPanel(YuvRenderer.YuvColorMode.Yuv420P, root, (uint)bmp.Dimensions.Width, (uint)bmp.Dimensions.Height);
            Task.Run(() =>
            {
                while (true)
                {
                    yuvRenderer.Render(bmp.Buffers[0].Buffer, bmp.Buffers[1].Buffer, bmp.Buffers[2].Buffer, bmp.Buffers[0].Pitch, bmp.Buffers[1].Pitch);
                }
            });
        }
    }
}
