#if _MSC_VER
#else
#error "Unknown platform!"
#endif

#define EXPORT_API __declspec(dllexport)

// Graphics device identifiers in Unity
enum GfxDeviceRenderer
{
	kGfxRendererOpenGL = 0,			// OpenGL
	kGfxRendererD3D9,				// Direct3D 9
	kGfxRendererD3D11,				// Direct3D 11
	kGfxRendererGCM,				// Sony PlayStation 3 GCM
	kGfxRendererNull,				// "null" device (used in batch mode)
	kGfxRendererHollywood,			// Nintendo Wii
	kGfxRendererXenon,				// Xbox 360
	kGfxRendererOpenGLES_Obsolete,
	kGfxRendererOpenGLES20Mobile,	// OpenGL ES 2.0
	kGfxRendererMolehill_Obsolete,
	kGfxRendererOpenGLES20Desktop_Obsolete,
	kGfxRendererOpenGLES30,			// OpenGL ES 3.0
	kGfxRendererCount
};


// Event types for UnitySetGraphicsDevice
enum GfxDeviceEventType {
	kGfxDeviceEventInitialize = 0,
	kGfxDeviceEventShutdown,
	kGfxDeviceEventBeforeReset,
	kGfxDeviceEventAfterReset,
};


// If exported by a plugin, this function will be called when graphics device is created, destroyed,
// before it's being reset (i.e. resolution changed), after it's being reset, etc.
extern "C" void EXPORT_API UnitySetGraphicsDevice(void* device, int deviceType, int eventType);

// If exported by a plugin, this function will be called for GL.IssuePluginEvent script calls.
// The function will be called on a rendering thread; note that when multithreaded rendering is used,
// the rendering thread WILL BE DIFFERENT from the thread that all scripts & other game logic happens!
// You have to ensure any synchronization with other plugin script calls is properly done by you.
extern "C" void EXPORT_API UnityRenderEvent(int eventID);

