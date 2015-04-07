# Yuv Renderer for WinRT

This project provides a fast way to render YUV frames (coming from still images or videos) in a WinRT Xaml app.
It leverages the GPU to render frames in different YUV formats (for now, YUV420P and NV12 are supported, as they seem to be the most common output formats from JPEG/H264 decoders).
It also provides an easy way to decode individual H264 frames using Media Foundation and DXVA (DirectX Video acceleration), in order to render video feeds coming from H264 encoders (like IP cameras, Drones, ...) with the lowest CPU overhead and the lowest latency as possible directly onto a xaml SwapchainPanel.

## Platforms

Both Windows 8.1 and Windows Phone 8.1 are supported.
Live H264 decoding performance depends on 2 GPU related factors : level of support of DXVA, and support of the DXGI_FORMAT_R8G8_UNORM texture format.
So, for example, in some micro benchmarks I've done, a Lumia 930 can sustain Full HD H264 decoding and rendering at native resolution at 60fps, where the Lumia 520 only renders at 15 fps on the same video.
I will do further investigations on performance on low-end devices.

Windows 10 support is also coming soon.

## Usage - YuvRendering

The YuvRenderer can be created either on top of a SurfaceImageSource or on a SwapChainPanel:
```C#
var yuvRenderer = YuvRenderer.YuvD3DRenderer.CreateForSwapChainPanel(scPanel, sourceWidth, sourceHeight, YuvRenderer.StretchMode.Uniform);
// or
var yuvRenderer = YuvRenderer.YuvD3DRenderer.CreateForD3DImageSource(imgSourceHeight, imgSourceWidth, imgSourceHeight, sourceWidth, sourceHeight, YuvRenderer.StretchMode.Uniform);
```
Once done, you can call the RenderYuv420P or RenderNV12 various overloads to render a frame.

### note on threading
The render* methods should be called always on the same thread. For a renderer created with CreateForD3DImageSource, it must be the UI thread.
For a renderer created with CreateForSwapChainPanel you should prefer rendering from a background thread. This way, if you put an heavy load on the renderer (like playing a 60fps Full HD video), you won't compete to much with other xaml engine related orders coming on the UI thread.

## Usage - H264 Live Decoder
The live h264 decoder works in collaboration with a YuvRenderer created for a SwapChainPanel.
So you create it this way :
```C#
var liveDecoder = new YuvRenderer.H264LiveDecoder(yuvRenderer);
```
The YUV renderer must have been initialized with the width / height of the video.

After creating the decoder, you can start the rendering thread and submit frames (from a background thread) :

```C#
liveDecoder.StartRenderingLoop();
while (hasFramesToRender)
{
	liveDecoder.SubmitH264Frame(byteArray);
}
liveDecoder.StopRenderingLoop();
```
H264 frames must be in the H264 annex b format (most commonly used by H264 live streams).
