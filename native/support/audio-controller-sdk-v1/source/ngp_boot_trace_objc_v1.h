/* Own V4 trace-only Objective-C helpers. Each helper reads UIKit state inside
 * its own @try so a diagnostic read can never raise into, or be handled by,
 * a host @catch. Helpers record; they never change UIKit or controller state.
 * Geometry values are UIKit points rounded to integers; they describe frames,
 * not measured pixels or compositing. */
#ifndef NGP_BOOT_TRACE_OBJC_V1_H
#define NGP_BOOT_TRACE_OBJC_V1_H
#import <UIKit/UIKit.h>
#include <math.h>
#include <string.h>
#include "ngp_boot_trace_v1.h"
#define NGP_BOOT_TRACE_EXPECTED_BUILD 3u
/* Arguments must be locals, ivars, constants or synthesized getters only. */
#define NGPBT(ev,a,b,c,d) ngp_boot_trace_event_v1(NGP_BT_##ev,(int64_t)(a),(int64_t)(b),(int64_t)(c),(int64_t)(d))
static inline int64_t ngpbt_pt(CGFloat v){return isfinite(v)&&fabs(v)<1e12?(int64_t)llround(v):INT64_MIN;}
static inline void ngpbt_exception(uint32_t event,NSException *exception){
 @try {
  const char *name=exception.name.UTF8String,*reason=exception.reason.UTF8String;
  ngp_boot_trace_exception_v1(event,name,name?strnlen(name,256):0,reason,reason?strnlen(reason,512):0);
 }@catch(NSException *again){(void)again;ngp_boot_trace_exception_v1(event,NULL,0,NULL,0);}
}
/* a=hidden|key<<1|visible-root<<2, b/c=bounds points, d=interface orientation or -1. */
static inline void ngpbt_window(uint32_t event,UIWindow *window){
 @try {
  UIWindowScene *scene=window.windowScene;CGRect b=window.bounds;
  int64_t flags=(window.hidden?1:0)|(window.isKeyWindow?2:0)|(window.rootViewController.viewIfLoaded.window==window&&window?4:0);
  ngp_boot_trace_event_v1(event,flags,ngpbt_pt(b.size.width),ngpbt_pt(b.size.height),scene?(int64_t)scene.effectiveGeometry.interfaceOrientation:-1);
 }@catch(NSException *e){(void)e;ngp_boot_trace_event_v1(event,-1,-1,-1,-1);}
}
/* a/b=root bounds, c/d=label frame size. */
static inline void ngpbt_view_geometry(uint32_t event,UIView *root,UIView *label){
 @try {
  CGRect r=root.bounds,l=label.frame;
  ngp_boot_trace_event_v1(event,ngpbt_pt(r.size.width),ngpbt_pt(r.size.height),ngpbt_pt(l.size.width),ngpbt_pt(l.size.height));
 }@catch(NSException *e){(void)e;ngp_boot_trace_event_v1(event,-1,-1,-1,-1);}
}
/* a=root-is-controller, b=window hidden, c=scene activation, d=application state. */
static inline void ngpbt_start_gate(UIWindow *window,UIWindowScene *scene,UIViewController *controller){
 @try {
  ngp_boot_trace_event_v1(NGP_BT_START_GATE,window.rootViewController==controller,window.hidden,
   scene?(int64_t)scene.activationState:-1,(int64_t)UIApplication.sharedApplication.applicationState);
 }@catch(NSException *e){(void)e;ngp_boot_trace_event_v1(NGP_BT_START_GATE,-1,-1,-1,-1);}
}
/* a=current interface orientation, b=previous, c=start requested, d=0. */
static inline void ngpbt_coord(UIWindowScene *scene,UIInterfaceOrientation previous,BOOL startRequested){
 @try {
  ngp_boot_trace_event_v1(NGP_BT_COORD_UPDATE,(int64_t)scene.effectiveGeometry.interfaceOrientation,(int64_t)previous,startRequested,0);
 }@catch(NSException *e){(void)e;ngp_boot_trace_event_v1(NGP_BT_COORD_UPDATE,-1,(int64_t)previous,startRequested,0);}
}
#endif
