// WebRTCQtCompat.h
#pragma once

// 1) Windows 头防护：减少宏污染，确保 winsock2 优先
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

// winsock2 必须在任何 Qt/Windows 头之前包含
#include <winsock2.h>
#include <ws2tcpip.h>

// 2) 提供宏包裹，临时禁用可能干扰 WebRTC 的宏（Qt 的 signals/slots，Windows 的 interface）
// 用法：
//   BEGIN_WEBRTC_INCLUDES
//   #include "api/peer_connection_interface.h"
//   #include "rtc_base/thread.h"
//   END_WEBRTC_INCLUDES

// BEGIN：压栈并 undef
#ifdef signals
#  pragma push_macro("signals")
#  define WEBRTCQT_HAS_SIGNALS_MACRO 1
#  undef signals
#endif
#ifdef slots
#  pragma push_macro("slots")
#  define WEBRTCQT_HAS_SLOTS_MACRO 1
#  undef slots
#endif
#ifdef interface  // Windows 有时会定义 #define interface struct
#  pragma push_macro("interface")
#  define WEBRTCQT_HAS_INTERFACE_MACRO 1
#  undef interface
#endif

// 有些环境里居然会定义 Args 宏，显式清掉
#ifdef Args
#  pragma push_macro("Args")
#  define WEBRTCQT_HAS_ARGS_MACRO 1
#  undef Args
#endif

// 宏：开始包含 WebRTC 头
#define BEGIN_WEBRTC_INCLUDES /* no-op for readability */
#define END_WEBRTC_INCLUDES   WEBRTCQT_RestoreMacros()

// 3) 恢复宏的内部实现
inline void WEBRTCQT_RestoreMacros() {
  // 用 pop_macro 恢复之前的宏状态
  // 注意：pop_macro 必须在同一个翻译单元里调用
#ifdef WEBRTCQT_HAS_SIGNALS_MACRO
#  pragma pop_macro("signals")
#  undef WEBRTCQT_HAS_SIGNALS_MACRO
#endif
#ifdef WEBRTCQT_HAS_SLOTS_MACRO
#  pragma pop_macro("slots")
#  undef WEBRTCQT_HAS_SLOTS_MACRO
#endif
#ifdef WEBRTCQT_HAS_INTERFACE_MACRO
#  pragma pop_macro("interface")
#  undef WEBRTCQT_HAS_INTERFACE_MACRO
#endif
#ifdef WEBRTCQT_HAS_ARGS_MACRO
#  pragma pop_macro("Args")
#  undef WEBRTCQT_HAS_ARGS_MACRO
#endif
}
