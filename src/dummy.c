//#include <GLES2/gl2.h>
#include "/opt/vc/include/IL/OMX_Core.h"
#include "/opt/vc/include/interface/vcos/vcos.h"
#include "/opt/vc/include/interface/vctypes/vc_image_types.h"
#include "/opt/vc/include/interface/vmcs_host/vc_dispservice_x_defs.h"
#include "/opt/vc/include/interface/vmcs_host/vc_dispmanx_types.h"
#include "/opt/vc/include/interface/vchi/vchi.h"
#include "/opt/vc/include/interface/vmcs_host/vchost_platform_config.h"
#include "/opt/vc/include/interface/vchi/vchi.h"
#include "/opt/vc/include/vcinclude/common.h"
#include "/opt/vc/include/interface/vcos/vcos.h"
#include "/opt/vc/include/interface/vchi/vchi.h"
#include "/opt/vc/include/interface/vmcs_host/vc_tvservice_defs.h"
#include "/opt/vc/include/interface/vmcs_host/vc_hdmi.h"
#include "/opt/vc/include/interface/vmcs_host/vc_sdtv.h"
#include "/opt/vc/include/EGL/egl.h"
#include "/opt/vc/include/EGL/eglext.h"
#include "/opt/vc/include/EGL/eglplatform.h"
#include "/opt/vc/include/GLES2/gl2.h"
#include "/opt/vc/include/GLES2/gl2platform.h"
#include "/opt/vc/include/KHR/khrplatform.h"
#include "/opt/vc/include/GLES/gl.h"

//void glScalef (GLfloat x, GLfloat y, GLfloat z){};
//void glTranslatef (GLfloat x, GLfloat y, GLfloat z){};
//void glRotatef (GLfloat angle, GLfloat x, GLfloat y, GLfloat z){};


OMX_API OMX_ERRORTYPE OMX_APIENTRY OMX_Init(void){return 0;};
OMX_API OMX_ERRORTYPE OMX_APIENTRY OMX_Deinit(void){return 0;};
OMX_API OMX_ERRORTYPE OMX_APIENTRY OMX_ComponentNameEnum(
		OMX_OUT OMX_STRING cComponentName,
		OMX_IN  OMX_U32 nNameLength,
		OMX_IN  OMX_U32 nIndex){return 0;};
OMX_API OMX_ERRORTYPE OMX_APIENTRY OMX_GetHandle( 
		OMX_OUT OMX_HANDLETYPE* pHandle, 
		OMX_IN  OMX_STRING cComponentName, 
		OMX_IN  OMX_PTR pAppData, 
		OMX_IN  OMX_CALLBACKTYPE* pCallBacks){return 0;};
OMX_API OMX_ERRORTYPE OMX_APIENTRY OMX_FreeHandle(OMX_IN  OMX_HANDLETYPE hComponent);
OMX_API OMX_ERRORTYPE OMX_APIENTRY OMX_SetupTunnel(
		OMX_IN  OMX_HANDLETYPE hOutput,
		OMX_IN  OMX_U32 nPortOutput,
		OMX_IN  OMX_HANDLETYPE hInput,OMX_IN  OMX_U32 nPortInput){return 0;};
OMX_API OMX_ERRORTYPE OMX_APIENTRY OMX_FreeHandle(OMX_IN  OMX_HANDLETYPE hComponent){return 0;};
		
VCHPRE_ int VCHPOST_ vc_dispman_init( void );
VCHPRE_ void VCHPOST_ vc_dispmanx_stop( void );
VCHPRE_ int VCHPOST_ vc_dispmanx_rect_set( VC_RECT_T *rect, uint32_t x_offset, uint32_t y_offset, uint32_t width, uint32_t height );
VCHPRE_ DISPMANX_RESOURCE_HANDLE_T VCHPOST_ vc_dispmanx_resource_create( VC_IMAGE_TYPE_T type, uint32_t width, uint32_t height, uint32_t *native_image_handle );
VCHPRE_ int VCHPOST_ vc_dispmanx_resource_write_data( DISPMANX_RESOURCE_HANDLE_T res, VC_IMAGE_TYPE_T src_type, int src_pitch, void * src_address, const VC_RECT_T * rect );
VCHPRE_ int VCHPOST_ vc_dispmanx_resource_write_data_handle( DISPMANX_RESOURCE_HANDLE_T res, VC_IMAGE_TYPE_T src_type, int src_pitch, VCHI_MEM_HANDLE_T handle, uint32_t offset, const VC_RECT_T * rect );
VCHPRE_ int VCHPOST_ vc_dispmanx_resource_read_data(DISPMANX_RESOURCE_HANDLE_T handle, const VC_RECT_T* p_rect, void *   dst_address, uint32_t dst_pitch );
VCHPRE_ int VCHPOST_ vc_dispmanx_resource_delete( DISPMANX_RESOURCE_HANDLE_T res );
VCHPRE_ DISPMANX_DISPLAY_HANDLE_T VCHPOST_ vc_dispmanx_display_open( uint32_t device );
VCHPRE_ DISPMANX_DISPLAY_HANDLE_T VCHPOST_ vc_dispmanx_display_open_mode( uint32_t device, uint32_t mode );
VCHPRE_ DISPMANX_DISPLAY_HANDLE_T VCHPOST_ vc_dispmanx_display_open_offscreen( DISPMANX_RESOURCE_HANDLE_T dest, DISPMANX_TRANSFORM_T orientation );
VCHPRE_ int VCHPOST_ vc_dispmanx_display_reconfigure( DISPMANX_DISPLAY_HANDLE_T display, uint32_t mode );
VCHPRE_ int VCHPOST_ vc_dispmanx_display_set_destination( DISPMANX_DISPLAY_HANDLE_T display, DISPMANX_RESOURCE_HANDLE_T dest );
VCHPRE_ int VCHPOST_ vc_dispmanx_display_set_background( DISPMANX_UPDATE_HANDLE_T update, DISPMANX_DISPLAY_HANDLE_T display, uint8_t red, uint8_t green, uint8_t blue );
VCHPRE_ int VCHPOST_ vc_dispmanx_display_get_info( DISPMANX_DISPLAY_HANDLE_T display, DISPMANX_MODEINFO_T * pinfo );
VCHPRE_ int VCHPOST_ vc_dispmanx_display_close( DISPMANX_DISPLAY_HANDLE_T display );
VCHPRE_ DISPMANX_UPDATE_HANDLE_T VCHPOST_ vc_dispmanx_update_start( int32_t priority );
VCHPRE_ DISPMANX_ELEMENT_HANDLE_T VCHPOST_ vc_dispmanx_element_add ( DISPMANX_UPDATE_HANDLE_T update, DISPMANX_DISPLAY_HANDLE_T display, int32_t layer, const VC_RECT_T *dest_rect, DISPMANX_RESOURCE_HANDLE_T src, const VC_RECT_T *src_rect, DISPMANX_PROTECTION_T protection, VC_DISPMANX_ALPHA_T *alpha, DISPMANX_CLAMP_T *clamp, DISPMANX_TRANSFORM_T transform );
VCHPRE_ int VCHPOST_ vc_dispmanx_element_change_source( DISPMANX_UPDATE_HANDLE_T update, DISPMANX_ELEMENT_HANDLE_T element, DISPMANX_RESOURCE_HANDLE_T src );
VCHPRE_ int VCHPOST_ vc_dispmanx_element_change_layer ( DISPMANX_UPDATE_HANDLE_T update, DISPMANX_ELEMENT_HANDLE_T element, int32_t layer );
VCHPRE_ int VCHPOST_ vc_dispmanx_element_modified( DISPMANX_UPDATE_HANDLE_T update, DISPMANX_ELEMENT_HANDLE_T element, const VC_RECT_T * rect );
VCHPRE_ int VCHPOST_ vc_dispmanx_element_remove( DISPMANX_UPDATE_HANDLE_T update, DISPMANX_ELEMENT_HANDLE_T element );
VCHPRE_ int VCHPOST_ vc_dispmanx_update_submit( DISPMANX_UPDATE_HANDLE_T update, DISPMANX_CALLBACK_FUNC_T cb_func, void *cb_arg );
VCHPRE_ int VCHPOST_ vc_dispmanx_update_submit_sync( DISPMANX_UPDATE_HANDLE_T update );
VCHPRE_ int VCHPOST_ vc_dispmanx_query_image_formats( uint32_t *supported_formats );
VCHPRE_ int VCHPOST_ vc_dispmanx_element_change_attributes( DISPMANX_UPDATE_HANDLE_T update, 
                                                            DISPMANX_ELEMENT_HANDLE_T element,
                                                            uint32_t change_flags,
                                                            int32_t layer,
                                                            uint8_t opacity,
                                                            const VC_RECT_T *dest_rect,
                                                            const VC_RECT_T *src_rect,
                                                            DISPMANX_RESOURCE_HANDLE_T mask,
                                                            DISPMANX_TRANSFORM_T transform );
VCHPRE_ uint32_t VCHPOST_ vc_dispmanx_resource_get_image_handle( DISPMANX_RESOURCE_HANDLE_T res);
VCHPRE_ void VCHPOST_ vc_vchi_dispmanx_init (VCHI_INSTANCE_T initialise_instance, VCHI_CONNECTION_T **connections, uint32_t num_connections );
VCHPRE_ int VCHPOST_ vc_dispmanx_snapshot( DISPMANX_DISPLAY_HANDLE_T display, DISPMANX_RESOURCE_HANDLE_T snapshot_resource, DISPMANX_TRANSFORM_T transform );
VCHPRE_ int VCHPOST_ vc_dispmanx_resource_set_palette( DISPMANX_RESOURCE_HANDLE_T handle, void * src_address, int offset, int size);
VCHPRE_ int VCHPOST_ vc_dispmanx_vsync_callback( DISPMANX_DISPLAY_HANDLE_T display, DISPMANX_CALLBACK_FUNC_T cb_func, void *cb_arg );


VCHPRE_ void VCHPOST_ vc_vchi_gencmd_init(VCHI_INSTANCE_T initialise_instance, VCHI_CONNECTION_T **connections, uint32_t num_connections ){};
VCHPRE_ int VCHPOST_ vc_gencmd_init(void){return 0;};
VCHPRE_ void VCHPOST_ vc_gencmd_stop(void){};
VCHPRE_ int VCHPOST_ vc_gencmd_send( const char *format, ... ){return 0;};
VCHPRE_ int VCHPOST_ vc_gencmd(char *response, int maxlen, const char *format, ...){return 0;};
VCHPRE_ int VCHPOST_ vc_gencmd_string_property(char *text, const char *property, char **value, int *length){return 0;};
VCHPRE_ int VCHPOST_ vc_gencmd_number_property(char *text, const char *property, int *number){return 0;};
VCHPRE_ int VCHPOST_ vc_gencmd_until( char        *cmd,
                                      const char  *property,
                                      char        *value,
                                      const char  *error_string,
                                      int         timeout){return 0;};  
									  
VCHPRE_ void vc_vchi_tv_stop( void ){};	
VCHPRE_ void vc_vchi_cec_stop( void ){};

void bcm_host_init(void){};
void bcm_host_deinit(void){};

int32_t graphics_get_display_size( const uint16_t display_number,
                                                    uint32_t *width,
                                                    uint32_t *height){return 0;};

unsigned bcm_host_get_peripheral_address(void){return 0;};
unsigned bcm_host_get_peripheral_size(void){return 0;};
unsigned bcm_host_get_sdram_address(void){return 0;};

VCHPRE_ int VCHPOST_ vc_dispman_init( void ){return 0;};
// Stop the service from being used
VCHPRE_ void VCHPOST_ vc_dispmanx_stop( void ){};
// Set the entries in the rect structure
VCHPRE_ int VCHPOST_ vc_dispmanx_rect_set( VC_RECT_T *rect, uint32_t x_offset, uint32_t y_offset, uint32_t width, uint32_t height ){return 0;};
// Resources
// Create a new resource
VCHPRE_ DISPMANX_RESOURCE_HANDLE_T VCHPOST_ vc_dispmanx_resource_create( VC_IMAGE_TYPE_T type, uint32_t width, uint32_t height, uint32_t *native_image_handle ){return 0;};
// Write the bitmap data to VideoCore memory
VCHPRE_ int VCHPOST_ vc_dispmanx_resource_write_data( DISPMANX_RESOURCE_HANDLE_T res, VC_IMAGE_TYPE_T src_type, int src_pitch, void * src_address, const VC_RECT_T * rect ){return 0;};
VCHPRE_ int VCHPOST_ vc_dispmanx_resource_write_data_handle( DISPMANX_RESOURCE_HANDLE_T res, VC_IMAGE_TYPE_T src_type, int src_pitch, VCHI_MEM_HANDLE_T handle, uint32_t offset, const VC_RECT_T * rect ){return 0;};
VCHPRE_ int VCHPOST_ vc_dispmanx_resource_read_data(
                              DISPMANX_RESOURCE_HANDLE_T handle,
                              const VC_RECT_T* p_rect,
                              void *   dst_address,
                              uint32_t dst_pitch ){return 0;};
// Delete a resource
VCHPRE_ int VCHPOST_ vc_dispmanx_resource_delete( DISPMANX_RESOURCE_HANDLE_T res ){return 0;};

// Displays
// Opens a display on the given device
VCHPRE_ DISPMANX_DISPLAY_HANDLE_T VCHPOST_ vc_dispmanx_display_open( uint32_t device ){return 0;};
// Opens a display on the given device in the request mode
VCHPRE_ DISPMANX_DISPLAY_HANDLE_T VCHPOST_ vc_dispmanx_display_open_mode( uint32_t device, uint32_t mode ){return 0;};
// Open an offscreen display
VCHPRE_ DISPMANX_DISPLAY_HANDLE_T VCHPOST_ vc_dispmanx_display_open_offscreen( DISPMANX_RESOURCE_HANDLE_T dest, DISPMANX_TRANSFORM_T orientation ){return 0;};
// Change the mode of a display
VCHPRE_ int VCHPOST_ vc_dispmanx_display_reconfigure( DISPMANX_DISPLAY_HANDLE_T display, uint32_t mode ){return 0;};
// Sets the desstination of the display to be the given resource
VCHPRE_ int VCHPOST_ vc_dispmanx_display_set_destination( DISPMANX_DISPLAY_HANDLE_T display, DISPMANX_RESOURCE_HANDLE_T dest ){return 0;};
// Set the background colour of the display
VCHPRE_ int VCHPOST_ vc_dispmanx_display_set_background( DISPMANX_UPDATE_HANDLE_T update, DISPMANX_DISPLAY_HANDLE_T display,
                                                                       uint8_t red, uint8_t green, uint8_t blue ){return 0;};
// get the width, height, frame rate and aspect ratio of the display
VCHPRE_ int VCHPOST_ vc_dispmanx_display_get_info( DISPMANX_DISPLAY_HANDLE_T display, DISPMANX_MODEINFO_T * pinfo ){return 0;};
// Closes a display
VCHPRE_ int VCHPOST_ vc_dispmanx_display_close( DISPMANX_DISPLAY_HANDLE_T display ){return 0;};

// Updates
// Start a new update, DISPMANX_NO_HANDLE on error
VCHPRE_ DISPMANX_UPDATE_HANDLE_T VCHPOST_ vc_dispmanx_update_start( int32_t priority ){return 0;};
// Add an elment to a display as part of an update
VCHPRE_ DISPMANX_ELEMENT_HANDLE_T VCHPOST_ vc_dispmanx_element_add ( DISPMANX_UPDATE_HANDLE_T update, DISPMANX_DISPLAY_HANDLE_T display,
                                                                     int32_t layer, const VC_RECT_T *dest_rect, DISPMANX_RESOURCE_HANDLE_T src,
                                                                     const VC_RECT_T *src_rect, DISPMANX_PROTECTION_T protection, 
                                                                     VC_DISPMANX_ALPHA_T *alpha,
                                                                     DISPMANX_CLAMP_T *clamp, DISPMANX_TRANSFORM_T transform ){return 0;};
// Change the source image of a display element
VCHPRE_ int VCHPOST_ vc_dispmanx_element_change_source( DISPMANX_UPDATE_HANDLE_T update, DISPMANX_ELEMENT_HANDLE_T element,
                                                        DISPMANX_RESOURCE_HANDLE_T src ){return 0;};
// Change the layer number of a display element
VCHPRE_ int VCHPOST_ vc_dispmanx_element_change_layer ( DISPMANX_UPDATE_HANDLE_T update, DISPMANX_ELEMENT_HANDLE_T element,
                                                        int32_t layer ){return 0;};
// Signal that a region of the bitmap has been modified
VCHPRE_ int VCHPOST_ vc_dispmanx_element_modified( DISPMANX_UPDATE_HANDLE_T update, DISPMANX_ELEMENT_HANDLE_T element, const VC_RECT_T * rect ){return 0;};
// Remove a display element from its display
VCHPRE_ int VCHPOST_ vc_dispmanx_element_remove( DISPMANX_UPDATE_HANDLE_T update, DISPMANX_ELEMENT_HANDLE_T element ){return 0;};
// Ends an update
VCHPRE_ int VCHPOST_ vc_dispmanx_update_submit( DISPMANX_UPDATE_HANDLE_T update, DISPMANX_CALLBACK_FUNC_T cb_func, void *cb_arg ){return 0;};
// End an update and wait for it to complete
VCHPRE_ int VCHPOST_ vc_dispmanx_update_submit_sync( DISPMANX_UPDATE_HANDLE_T update ){return 0;};
// Query the image formats supported in the VMCS build
VCHPRE_ int VCHPOST_ vc_dispmanx_query_image_formats( uint32_t *supported_formats ){return 0;};

//New function added to VCHI to change attributes, set_opacity does not work there.
VCHPRE_ int VCHPOST_ vc_dispmanx_element_change_attributes( DISPMANX_UPDATE_HANDLE_T update, 
                                                            DISPMANX_ELEMENT_HANDLE_T element,
                                                            uint32_t change_flags,
                                                            int32_t layer,
                                                            uint8_t opacity,
                                                            const VC_RECT_T *dest_rect,
                                                            const VC_RECT_T *src_rect,
                                                            DISPMANX_RESOURCE_HANDLE_T mask,
                                                            DISPMANX_TRANSFORM_T transform ){return 0;};

//xxx hack to get the image pointer from a resource handle, will be obsolete real soon
VCHPRE_ uint32_t VCHPOST_ vc_dispmanx_resource_get_image_handle( DISPMANX_RESOURCE_HANDLE_T res){return 0;};

//Call this instead of vc_dispman_init
VCHPRE_ void VCHPOST_ vc_vchi_dispmanx_init (VCHI_INSTANCE_T initialise_instance, VCHI_CONNECTION_T **connections, uint32_t num_connections ){return;};

// Take a snapshot of a display in its current state.
// This call may block for a time; when it completes, the snapshot is ready.
// only transform=0 is supported
VCHPRE_ int VCHPOST_ vc_dispmanx_snapshot( DISPMANX_DISPLAY_HANDLE_T display, 
                                           DISPMANX_RESOURCE_HANDLE_T snapshot_resource, 
                                           DISPMANX_TRANSFORM_T transform ){return 0;};

// Set the resource palette (for VC_IMAGE_4BPP and VC_IMAGE_8BPP)
VCHPRE_ int VCHPOST_ vc_dispmanx_resource_set_palette( DISPMANX_RESOURCE_HANDLE_T handle, 
                                                      void * src_address, int offset, int size){return 0;};

// Start triggering callbacks synced to vsync
VCHPRE_ int VCHPOST_ vc_dispmanx_vsync_callback( DISPMANX_DISPLAY_HANDLE_T display, DISPMANX_CALLBACK_FUNC_T cb_func, void *cb_arg ){return 0;};

EGLAPI EGLint EGLAPIENTRY eglGetError(void){return 0;};

EGLAPI EGLDisplay EGLAPIENTRY eglGetDisplay(EGLNativeDisplayType display_id){return 0;};
EGLAPI EGLBoolean EGLAPIENTRY eglInitialize(EGLDisplay dpy, EGLint *major, EGLint *minor){return 0;};
EGLAPI EGLBoolean EGLAPIENTRY eglTerminate(EGLDisplay dpy){return 0;};

EGLAPI const char * EGLAPIENTRY eglQueryString(EGLDisplay dpy, EGLint name);

EGLAPI EGLBoolean EGLAPIENTRY eglGetConfigs(EGLDisplay dpy, EGLConfig *configs,
			 EGLint config_size, EGLint *num_config);
EGLAPI EGLBoolean EGLAPIENTRY eglChooseConfig(EGLDisplay dpy, const EGLint *attrib_list,
			   EGLConfig *configs, EGLint config_size,
			   EGLint *num_config);
EGLAPI EGLBoolean EGLAPIENTRY eglGetConfigAttrib(EGLDisplay dpy, EGLConfig config,
			      EGLint attribute, EGLint *value);

EGLAPI EGLSurface EGLAPIENTRY eglCreateWindowSurface(EGLDisplay dpy, EGLConfig config,
				  EGLNativeWindowType win,
				  const EGLint *attrib_list){return 0;};
EGLAPI EGLSurface EGLAPIENTRY eglCreatePbufferSurface(EGLDisplay dpy, EGLConfig config,
				   const EGLint *attrib_list);
EGLAPI EGLSurface EGLAPIENTRY eglCreatePixmapSurface(EGLDisplay dpy, EGLConfig config,
				  EGLNativePixmapType pixmap,
				  const EGLint *attrib_list);
EGLAPI EGLBoolean EGLAPIENTRY eglDestroySurface(EGLDisplay dpy, EGLSurface surface){return 0;};
EGLAPI EGLBoolean EGLAPIENTRY eglQuerySurface(EGLDisplay dpy, EGLSurface surface,
			   EGLint attribute, EGLint *value){return 0;};

EGLAPI EGLBoolean EGLAPIENTRY eglBindAPI(EGLenum api);
EGLAPI EGLenum EGLAPIENTRY eglQueryAPI(void);

EGLAPI EGLBoolean EGLAPIENTRY eglWaitClient(void);

EGLAPI EGLBoolean EGLAPIENTRY eglReleaseThread(void);

EGLAPI EGLSurface EGLAPIENTRY eglCreatePbufferFromClientBuffer(
	      EGLDisplay dpy, EGLenum buftype, EGLClientBuffer buffer,
	      EGLConfig config, const EGLint *attrib_list);

EGLAPI EGLBoolean EGLAPIENTRY eglSurfaceAttrib(EGLDisplay dpy, EGLSurface surface,
			    EGLint attribute, EGLint value);
EGLAPI EGLBoolean EGLAPIENTRY eglBindTexImage(EGLDisplay dpy, EGLSurface surface, EGLint buffer);
EGLAPI EGLBoolean EGLAPIENTRY eglReleaseTexImage(EGLDisplay dpy, EGLSurface surface, EGLint buffer){return 0;};


EGLAPI EGLBoolean EGLAPIENTRY eglSwapInterval(EGLDisplay dpy, EGLint interval);


EGLAPI EGLContext EGLAPIENTRY eglCreateContext(EGLDisplay dpy, EGLConfig config,
			    EGLContext share_context,
			    const EGLint *attrib_list){return 0;};
EGLAPI EGLBoolean EGLAPIENTRY eglDestroyContext(EGLDisplay dpy, EGLContext ctx){return 0;};
EGLAPI EGLBoolean EGLAPIENTRY eglMakeCurrent(EGLDisplay dpy, EGLSurface draw,
			  EGLSurface read, EGLContext ctx){return 0;};

EGLAPI EGLContext EGLAPIENTRY eglGetCurrentContext(void){return 0;};
EGLAPI EGLSurface EGLAPIENTRY eglGetCurrentSurface(EGLint readdraw){return 0;};
EGLAPI EGLDisplay EGLAPIENTRY eglGetCurrentDisplay(void){return 0;};
EGLAPI EGLBoolean EGLAPIENTRY eglQueryContext(EGLDisplay dpy, EGLContext ctx,
			   EGLint attribute, EGLint *value){return 0;};

EGLAPI EGLBoolean EGLAPIENTRY eglWaitGL(void);
EGLAPI EGLBoolean EGLAPIENTRY eglWaitNative(EGLint engine);
EGLAPI EGLBoolean EGLAPIENTRY eglSwapBuffers(EGLDisplay dpy, EGLSurface surface){return 0;};
EGLAPI EGLBoolean EGLAPIENTRY eglCopyBuffers(EGLDisplay dpy, EGLSurface surface,
			  EGLNativePixmapType target){return 0;};

EGLAPI EGLImageKHR EGLAPIENTRY eglCreateImageKHR (EGLDisplay dpy, EGLContext ctx, EGLenum target, EGLClientBuffer buffer, const EGLint *attrib_list){return 0;};
EGLAPI EGLBoolean EGLAPIENTRY eglDestroyImageKHR (EGLDisplay dpy, EGLImageKHR image){return 0;};
EGLAPI EGLBoolean EGLAPIENTRY eglSaneChooseConfigBRCM(EGLDisplay dpy, const EGLint *attrib_list, EGLConfig *configs, EGLint config_size, EGLint *num_config){return 0;};

GL_APICALL void         GL_APIENTRY glActiveTexture (GLenum texture){};
GL_APICALL void         GL_APIENTRY glAttachShader (GLuint program, GLuint shader){};
GL_APICALL void         GL_APIENTRY glBindAttribLocation (GLuint program, GLuint index, const GLchar* name){};
GL_APICALL void         GL_APIENTRY glBindBuffer (GLenum target, GLuint buffer){};
GL_APICALL void         GL_APIENTRY glBindFramebuffer (GLenum target, GLuint framebuffer){};
GL_APICALL void         GL_APIENTRY glBindRenderbuffer (GLenum target, GLuint renderbuffer){};
GL_APICALL void         GL_APIENTRY glBindTexture (GLenum target, GLuint texture){};
GL_APICALL void         GL_APIENTRY glBlendColor (GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha){};
GL_APICALL void         GL_APIENTRY glBlendEquation ( GLenum mode ){};
GL_APICALL void         GL_APIENTRY glBlendEquationSeparate (GLenum modeRGB, GLenum modeAlpha){};
GL_APICALL void         GL_APIENTRY glBlendFunc (GLenum sfactor, GLenum dfactor){};
GL_APICALL void         GL_APIENTRY glBlendFuncSeparate (GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha){};
GL_APICALL void         GL_APIENTRY glBufferData (GLenum target, GLsizeiptr size, const GLvoid* data, GLenum usage){};
GL_APICALL void         GL_APIENTRY glBufferSubData (GLenum target, GLintptr offset, GLsizeiptr size, const GLvoid* data){};
GL_APICALL GLenum       GL_APIENTRY glCheckFramebufferStatus (GLenum target){return 0;};
GL_APICALL void         GL_APIENTRY glClear (GLbitfield mask){};
GL_APICALL void         GL_APIENTRY glClearColor (GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha){};
GL_APICALL void         GL_APIENTRY glClearDepthf (GLclampf depth){};
GL_APICALL void         GL_APIENTRY glClearStencil (GLint s){};
GL_APICALL void         GL_APIENTRY glColorMask (GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha){};
GL_APICALL void         GL_APIENTRY glCompileShader (GLuint shader){};
GL_APICALL void         GL_APIENTRY glCompressedTexImage2D (GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const GLvoid* data){};
GL_APICALL void         GL_APIENTRY glCompressedTexSubImage2D (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const GLvoid* data){};
GL_APICALL void         GL_APIENTRY glCopyTexImage2D (GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border){};
GL_APICALL void         GL_APIENTRY glCopyTexSubImage2D (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height){};
GL_APICALL GLuint       GL_APIENTRY glCreateProgram (void){return 0;};
GL_APICALL GLuint       GL_APIENTRY glCreateShader (GLenum type){return 0;};
GL_APICALL void         GL_APIENTRY glCullFace (GLenum mode){};
GL_APICALL void         GL_APIENTRY glDeleteBuffers (GLsizei n, const GLuint* buffers){};
GL_APICALL void         GL_APIENTRY glDeleteFramebuffers (GLsizei n, const GLuint* framebuffers){};
GL_APICALL void         GL_APIENTRY glDeleteProgram (GLuint program){};
GL_APICALL void         GL_APIENTRY glDeleteRenderbuffers (GLsizei n, const GLuint* renderbuffers){};
GL_APICALL void         GL_APIENTRY glDeleteShader (GLuint shader){};
GL_APICALL void         GL_APIENTRY glDeleteTextures (GLsizei n, const GLuint* textures){};
GL_APICALL void         GL_APIENTRY glDepthFunc (GLenum func){};
GL_APICALL void         GL_APIENTRY glDepthMask (GLboolean flag){};
GL_APICALL void         GL_APIENTRY glDepthRangef (GLclampf zNear, GLclampf zFar){};
GL_APICALL void         GL_APIENTRY glDetachShader (GLuint program, GLuint shader){};
GL_APICALL void         GL_APIENTRY glDisable (GLenum cap){};
GL_APICALL void         GL_APIENTRY glDisableVertexAttribArray (GLuint index){};
GL_APICALL void         GL_APIENTRY glDrawArrays (GLenum mode, GLint first, GLsizei count){};
GL_APICALL void         GL_APIENTRY glDrawElements (GLenum mode, GLsizei count, GLenum type, const GLvoid* indices){};
GL_APICALL void         GL_APIENTRY glEnable (GLenum cap){};
GL_APICALL void         GL_APIENTRY glEnableVertexAttribArray (GLuint index){};
GL_APICALL void         GL_APIENTRY glFinish (void){};
GL_APICALL void         GL_APIENTRY glFlush (void){};
GL_APICALL void         GL_APIENTRY glFramebufferRenderbuffer (GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer){};
GL_APICALL void         GL_APIENTRY glFramebufferTexture2D (GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level){};
GL_APICALL void         GL_APIENTRY glFrontFace (GLenum mode){};
GL_APICALL void         GL_APIENTRY glGenBuffers (GLsizei n, GLuint* buffers){};
GL_APICALL void         GL_APIENTRY glGenerateMipmap (GLenum target){};
GL_APICALL void         GL_APIENTRY glGenFramebuffers (GLsizei n, GLuint* framebuffers){};
GL_APICALL void         GL_APIENTRY glGenRenderbuffers (GLsizei n, GLuint* renderbuffers){};
GL_APICALL void         GL_APIENTRY glGenTextures (GLsizei n, GLuint* textures){};
GL_APICALL void         GL_APIENTRY glGetActiveAttrib (GLuint program, GLuint index, GLsizei bufsize, GLsizei* length, GLint* size, GLenum* type, GLchar* name){};
GL_APICALL void         GL_APIENTRY glGetActiveUniform (GLuint program, GLuint index, GLsizei bufsize, GLsizei* length, GLint* size, GLenum* type, GLchar* name){};
GL_APICALL void         GL_APIENTRY glGetAttachedShaders (GLuint program, GLsizei maxcount, GLsizei* count, GLuint* shaders){};
GL_APICALL int          GL_APIENTRY glGetAttribLocation (GLuint program, const GLchar* name){return 0;};
GL_APICALL void         GL_APIENTRY glGetBooleanv (GLenum pname, GLboolean* params){};
GL_APICALL void         GL_APIENTRY glGetBufferParameteriv (GLenum target, GLenum pname, GLint* params){};
GL_APICALL GLenum       GL_APIENTRY glGetError (void){return 0;};
GL_APICALL void         GL_APIENTRY glGetFloatv (GLenum pname, GLfloat* params){};
GL_APICALL void         GL_APIENTRY glGetFramebufferAttachmentParameteriv (GLenum target, GLenum attachment, GLenum pname, GLint* params){};
GL_APICALL void         GL_APIENTRY glGetIntegerv (GLenum pname, GLint* params){};
GL_APICALL void         GL_APIENTRY glGetProgramiv (GLuint program, GLenum pname, GLint* params){};
GL_APICALL void         GL_APIENTRY glGetProgramInfoLog (GLuint program, GLsizei bufsize, GLsizei* length, GLchar* infolog){};
GL_APICALL void         GL_APIENTRY glGetRenderbufferParameteriv (GLenum target, GLenum pname, GLint* params){};
GL_APICALL void         GL_APIENTRY glGetShaderiv (GLuint shader, GLenum pname, GLint* params){};
GL_APICALL void         GL_APIENTRY glGetShaderInfoLog (GLuint shader, GLsizei bufsize, GLsizei* length, GLchar* infolog){};
GL_APICALL void         GL_APIENTRY glGetShaderPrecisionFormat (GLenum shadertype, GLenum precisiontype, GLint* range, GLint* precision){};
GL_APICALL void         GL_APIENTRY glGetShaderSource (GLuint shader, GLsizei bufsize, GLsizei* length, GLchar* source){};
GL_APICALL const GLubyte* GL_APIENTRY glGetString (GLenum name){return 0;};
GL_APICALL void         GL_APIENTRY glGetTexParameterfv (GLenum target, GLenum pname, GLfloat* params){};
GL_APICALL void         GL_APIENTRY glGetTexParameteriv (GLenum target, GLenum pname, GLint* params){};
GL_APICALL void         GL_APIENTRY glGetUniformfv (GLuint program, GLint location, GLfloat* params){};
GL_APICALL void         GL_APIENTRY glGetUniformiv (GLuint program, GLint location, GLint* params){};
GL_APICALL int          GL_APIENTRY glGetUniformLocation (GLuint program, const GLchar* name){return 0;};
GL_APICALL void         GL_APIENTRY glGetVertexAttribfv (GLuint index, GLenum pname, GLfloat* params){};
GL_APICALL void         GL_APIENTRY glGetVertexAttribiv (GLuint index, GLenum pname, GLint* params){};
GL_APICALL void         GL_APIENTRY glGetVertexAttribPointerv (GLuint index, GLenum pname, GLvoid** pointer){};
GL_APICALL void         GL_APIENTRY glHint (GLenum target, GLenum mode){};
GL_APICALL GLboolean    GL_APIENTRY glIsBuffer (GLuint buffer){return 0;};
GL_APICALL GLboolean    GL_APIENTRY glIsEnabled (GLenum cap){return 0;};
GL_APICALL GLboolean    GL_APIENTRY glIsFramebuffer (GLuint framebuffer){return 0;};
GL_APICALL GLboolean    GL_APIENTRY glIsProgram (GLuint program){return 0;};
GL_APICALL GLboolean    GL_APIENTRY glIsRenderbuffer (GLuint renderbuffer){return 0;};
GL_APICALL GLboolean    GL_APIENTRY glIsShader (GLuint shader){return 0;};
GL_APICALL GLboolean    GL_APIENTRY glIsTexture (GLuint texture){return 0;};
GL_APICALL void         GL_APIENTRY glLineWidth (GLfloat width){};
GL_APICALL void         GL_APIENTRY glLinkProgram (GLuint program){};
GL_APICALL void         GL_APIENTRY glPixelStorei (GLenum pname, GLint param){};
GL_APICALL void         GL_APIENTRY glPolygonOffset (GLfloat factor, GLfloat units){};
GL_APICALL void         GL_APIENTRY glReadPixels (GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid* pixels){};
GL_APICALL void         GL_APIENTRY glReleaseShaderCompiler (void){};
GL_APICALL void         GL_APIENTRY glRenderbufferStorage (GLenum target, GLenum internalformat, GLsizei width, GLsizei height){};
GL_APICALL void         GL_APIENTRY glSampleCoverage (GLclampf value, GLboolean invert){};
GL_APICALL void         GL_APIENTRY glScissor (GLint x, GLint y, GLsizei width, GLsizei height){};
GL_APICALL void         GL_APIENTRY glShaderBinary (GLsizei n, const GLuint* shaders, GLenum binaryformat, const GLvoid* binary, GLsizei length){};
GL_APICALL void         GL_APIENTRY glShaderSource (GLuint shader, GLsizei count, const GLchar** string, const GLint* length){};
GL_APICALL void         GL_APIENTRY glStencilFunc (GLenum func, GLint ref, GLuint mask){};
GL_APICALL void         GL_APIENTRY glStencilFuncSeparate (GLenum face, GLenum func, GLint ref, GLuint mask){};
GL_APICALL void         GL_APIENTRY glStencilMask (GLuint mask){};
GL_APICALL void         GL_APIENTRY glStencilMaskSeparate (GLenum face, GLuint mask){};
GL_APICALL void         GL_APIENTRY glStencilOp (GLenum fail, GLenum zfail, GLenum zpass){};
GL_APICALL void         GL_APIENTRY glStencilOpSeparate (GLenum face, GLenum fail, GLenum zfail, GLenum zpass){};
GL_APICALL void         GL_APIENTRY glTexImage2D (GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid* pixels){};
GL_APICALL void         GL_APIENTRY glTexParameterf (GLenum target, GLenum pname, GLfloat param){};
GL_APICALL void         GL_APIENTRY glTexParameterfv (GLenum target, GLenum pname, const GLfloat* params){};
GL_APICALL void         GL_APIENTRY glTexParameteri (GLenum target, GLenum pname, GLint param){};
GL_APICALL void         GL_APIENTRY glTexParameteriv (GLenum target, GLenum pname, const GLint* params){};
GL_APICALL void         GL_APIENTRY glTexSubImage2D (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid* pixels){};
GL_APICALL void         GL_APIENTRY glUniform1f (GLint location, GLfloat x){};
GL_APICALL void         GL_APIENTRY glUniform1fv (GLint location, GLsizei count, const GLfloat* v){};
GL_APICALL void         GL_APIENTRY glUniform1i (GLint location, GLint x){};
GL_APICALL void         GL_APIENTRY glUniform1iv (GLint location, GLsizei count, const GLint* v){};
GL_APICALL void         GL_APIENTRY glUniform2f (GLint location, GLfloat x, GLfloat y){};
GL_APICALL void         GL_APIENTRY glUniform2fv (GLint location, GLsizei count, const GLfloat* v){};
GL_APICALL void         GL_APIENTRY glUniform2i (GLint location, GLint x, GLint y){};
GL_APICALL void         GL_APIENTRY glUniform2iv (GLint location, GLsizei count, const GLint* v){};
GL_APICALL void         GL_APIENTRY glUniform3f (GLint location, GLfloat x, GLfloat y, GLfloat z){};
GL_APICALL void         GL_APIENTRY glUniform3fv (GLint location, GLsizei count, const GLfloat* v){};
GL_APICALL void         GL_APIENTRY glUniform3i (GLint location, GLint x, GLint y, GLint z){};
GL_APICALL void         GL_APIENTRY glUniform3iv (GLint location, GLsizei count, const GLint* v){};
GL_APICALL void         GL_APIENTRY glUniform4f (GLint location, GLfloat x, GLfloat y, GLfloat z, GLfloat w){};
GL_APICALL void         GL_APIENTRY glUniform4fv (GLint location, GLsizei count, const GLfloat* v){};
GL_APICALL void         GL_APIENTRY glUniform4i (GLint location, GLint x, GLint y, GLint z, GLint w){};
GL_APICALL void         GL_APIENTRY glUniform4iv (GLint location, GLsizei count, const GLint* v){};
GL_APICALL void         GL_APIENTRY glUniformMatrix2fv (GLint location, GLsizei count, GLboolean transpose, const GLfloat* value){};
GL_APICALL void         GL_APIENTRY glUniformMatrix3fv (GLint location, GLsizei count, GLboolean transpose, const GLfloat* value){};
GL_APICALL void         GL_APIENTRY glUniformMatrix4fv (GLint location, GLsizei count, GLboolean transpose, const GLfloat* value){};
GL_APICALL void         GL_APIENTRY glUseProgram (GLuint program){};
GL_APICALL void         GL_APIENTRY glValidateProgram (GLuint program){};
GL_APICALL void         GL_APIENTRY glVertexAttrib1f (GLuint indx, GLfloat x){};
GL_APICALL void         GL_APIENTRY glVertexAttrib1fv (GLuint indx, const GLfloat* values){};
GL_APICALL void         GL_APIENTRY glVertexAttrib2f (GLuint indx, GLfloat x, GLfloat y){};
GL_APICALL void         GL_APIENTRY glVertexAttrib2fv (GLuint indx, const GLfloat* values){};
GL_APICALL void         GL_APIENTRY glVertexAttrib3f (GLuint indx, GLfloat x, GLfloat y, GLfloat z){};
GL_APICALL void         GL_APIENTRY glVertexAttrib3fv (GLuint indx, const GLfloat* values){};
GL_APICALL void         GL_APIENTRY glVertexAttrib4f (GLuint indx, GLfloat x, GLfloat y, GLfloat z, GLfloat w){};
GL_APICALL void         GL_APIENTRY glVertexAttrib4fv (GLuint indx, const GLfloat* values){};
GL_APICALL void         GL_APIENTRY glVertexAttribPointer (GLuint indx, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid* ptr){};
GL_APICALL void         GL_APIENTRY glViewport (GLint x, GLint y, GLsizei width, GLsizei height){};

GL_API void GL_APIENTRY glTexCoordPointer (GLint size, GLenum type, GLsizei stride, const GLvoid *pointer){};
GL_API void GL_APIENTRY glEnableClientState (GLenum array){};
GL_API void GL_APIENTRY glVertexPointer (GLint size, GLenum type, GLsizei stride, const GLvoid *pointer){};
GL_API void GL_APIENTRY glLoadIdentity (void){};
GL_API void GL_APIENTRY glScalef (GLfloat x, GLfloat y, GLfloat z){};
GL_API void GL_APIENTRY glTranslatef (GLfloat x, GLfloat y, GLfloat z){};
GL_API void GL_APIENTRY glRotatef (GLfloat angle, GLfloat x, GLfloat y, GLfloat z){};
GL_API void GL_APIENTRY glOrthof (GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat zNear, GLfloat zFar){};
GL_API void GL_APIENTRY glMatrixMode (GLenum mode){};
