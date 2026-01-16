import { useEffect, useRef, useState } from 'react';
import { useParams } from 'react-router-dom';
import { invoke } from '@tauri-apps/api/core';
import { getCurrentWindow } from '@tauri-apps/api/window';
import { Mic, MicOff, Volume2, X, Activity, Terminal } from 'lucide-react';

// Types
interface Member {
    id: string | number;
    name: string;
    avatar?: string;
}

interface RoomInfo {
    room_id: string;
    whip_url: string;
    whep_url: string;
    stream_name: string;
    group_id: string;
}

const IntercomWindow = () => {
    const { groupId } = useParams<{ groupId: string }>();
    const [statusLogs, setStatusLogs] = useState<{ msg: string; type: 'info' | 'success' | 'date' | 'error' | 'warn' }[]>([]);
    const [members, setMembers] = useState<Member[]>([]);
    const [isPublishing, setIsPublishing] = useState(false);
    const [intercomEnabled, setIntercomEnabled] = useState(false); // Can talk?
    const [currentUser, setCurrentUser] = useState<Member | null>(null);
    const [roomInfo, setRoomInfo] = useState<RoomInfo | null>(null);
    const [speakingUser, setSpeakingUser] = useState<string | null>(null);

    // Refs for WebRTC and WS
    const wsRef = useRef<WebSocket | null>(null);
    const publishPcRef = useRef<RTCPeerConnection | null>(null);
    const subscribePcRef = useRef<RTCPeerConnection | null>(null);
    const publishStreamRef = useRef<MediaStream | null>(null);
    const subscribeStreamRef = useRef<MediaStream | null>(null);
    const statusLogRef = useRef<HTMLDivElement>(null);
    const isTalkingRef = useRef(false); // Track active talking intent to handle async races

    const log = (msg: string, type: 'info' | 'success' | 'date' | 'error' | 'warn' = 'info') => {
        const time = new Date().toLocaleTimeString();
        setStatusLogs(prev => [...prev, { msg: `${time} ${msg}`, type }]);
        if (statusLogRef.current) {
            statusLogRef.current.scrollTop = statusLogRef.current.scrollHeight;
        }
    };

    // Initialize: Fetch Data -> Connect WS -> (Wait for room) -> Pull Stream
    useEffect(() => {
        const init = async () => {
            if (!groupId) return;

            // 1. Get User Info
            try {
                const uStr = localStorage.getItem('user_info');
                if (uStr) {
                    const u = JSON.parse(uStr);
                    setCurrentUser({
                        id: localStorage.getItem('teacher_unique_id') || u.teacher_unique_id,
                        name: u.name || u.strName || "我",
                        avatar: u.icon || u.face_url
                    });
                }
            } catch (e) { console.error(e); }

            // 2. Fetch Group Members & Temp Room (parallel)
            try {
                const token = JSON.parse(localStorage.getItem('user_info') || '{}').token || "";

                // Get Members
                invoke<string>('get_group_members', { groupId, token })
                    .then(async (resStr) => {
                        const res = JSON.parse(resStr);
                        if (res.data?.members) {
                            let mList = Array.isArray(res.data.members) ? res.data.members : [];

                            // Simplified Mapping - Use Backend Data Only
                            mList = mList.map((m: any) => {
                                console.log("[Debug] Raw Member:", m);
                                return {
                                    id: String(m.user_id || m.id || m.Member_Account),
                                    name: m.user_name || m.name || m.student_name || "未知",
                                    avatar: m.face_url || m.avatar || ""
                                };
                            });

                            setMembers(mList);

                            // Check initial room info from group_info if present
                            if (res.data.group_info?.temp_room) {
                                const tr = res.data.group_info.temp_room;
                                setRoomInfo({
                                    room_id: tr.room_id,
                                    whip_url: tr.whip_url || tr.publish_url,
                                    whep_url: tr.whep_url || tr.play_url,
                                    stream_name: tr.stream_name || tr.room_id,
                                    group_id: groupId
                                });
                            }
                        }
                    })
                    .catch(err => log(`获取成员失败: ${err}`, 'error'));

                // Fetch Temp Room explicit call (fallback/update)
                log(`[Debug] Calling fetch_temp_room with group_id: ${groupId}`, 'info');
                invoke<string>('fetch_temp_room', { groupId: groupId })
                    .then(async (resStr) => {
                        console.log("fetch_temp_room raw:", resStr);
                        const res = JSON.parse(resStr);

                        let tr = null;
                        if (res.code === 200) {
                            if (res.data) {
                                if (Array.isArray(res.data.rooms) && res.data.rooms.length > 0) {
                                    // Structure: { data: { rooms: [...] } }
                                    tr = res.data.rooms[0];
                                } else if (!Array.isArray(res.data) && res.data.room_id) {
                                    // Structure: { data: { room_id: ... } }
                                    tr = res.data;
                                } else if (Array.isArray(res.data) && res.data.length > 0) {
                                    // Structure: { data: [...] }
                                    tr = res.data[0];
                                }
                            }
                        }

                        if (tr) {
                            log(`获取房间信息成功: ${tr.room_id}`, 'success');
                            setRoomInfo({
                                room_id: tr.room_id,
                                whip_url: tr.whip_url || tr.publish_url,
                                whep_url: tr.whep_url || tr.play_url,
                                stream_name: tr.stream_name || tr.room_id,
                                group_id: groupId
                            });
                        } else {
                            log('未找到活跃房间，尝试开启对讲...', 'warn');

                            // Try to enable intercom (create room)
                            try {
                                log(`[Debug] Enabling intercom (creating room) for group_id: ${groupId}`, 'info');
                                const toggleResStr = await invoke<string>('toggle_group_intercom', {
                                    groupId: groupId,
                                    enable: true
                                });
                                console.log("toggle_group_intercom res:", toggleResStr);
                                const toggleRes = JSON.parse(toggleResStr);

                                if (toggleRes.code === 200) {
                                    log('对讲已开启，重新获取房间...', 'info');
                                    // Fetch again
                                    setTimeout(async () => {
                                        const retryResStr = await invoke<string>('fetch_temp_room', { groupId: groupId });
                                        const retryRes = JSON.parse(retryResStr);
                                        let r = null;
                                        if (retryRes.data?.rooms && retryRes.data.rooms.length > 0) {
                                            r = retryRes.data.rooms[0];
                                        } else if (retryRes.data && retryRes.data[0]) {
                                            r = retryRes.data[0];
                                        }

                                        if (r) {
                                            log(`房间创建成功 ID: ${r.room_id}`, 'success');
                                            setRoomInfo({
                                                room_id: r.room_id,
                                                whip_url: r.whip_url,
                                                whep_url: r.whep_url,
                                                stream_name: r.stream_name || r.room_id,
                                                group_id: groupId
                                            });
                                        }
                                    }, 500);
                                } else {
                                    log(`开启对讲失败: ${toggleRes.msg}`, 'error');
                                }
                            } catch (e: any) {
                                log(`开启对讲请求错误: ${e}`, 'error');
                            }
                        }
                    })
                    .catch(err => log(`获取房间失败: ${err}`, 'error'));

            } catch (e) {
                log(`初始化数据失败: ${e}`, 'error');
            }

            // 3. Connect WebSocket
            connectWebSocket();
        };

        init();



        return () => {
            // Cleanup
            if (wsRef.current) {
                wsRef.current.close();
                wsRef.current = null;
            }
            if (publishPcRef.current) {
                publishPcRef.current.close();
                publishPcRef.current = null;
            }
            if (subscribePcRef.current) {
                subscribePcRef.current.close();
                subscribePcRef.current = null;
            }
            if (publishStreamRef.current) {
                publishStreamRef.current.getTracks().forEach(t => t.stop());
                publishStreamRef.current = null;
            }
            // subscribeStreamRef tracks are remote, handled by PC close usually
        };
    }, [groupId]);

    // 4. Auto-pull stream when ready (Top Level)
    useEffect(() => {
        if (roomInfo?.whep_url && wsRef.current && wsRef.current.readyState === WebSocket.OPEN && !subscribePcRef.current) {
            log('检测到房间信息且WS已连接，自动拉流...', 'info');
            startPullStream(roomInfo.whep_url, roomInfo);
        }
    }, [roomInfo]); // Depends on roomInfo change (which happens after fetch)

    const connectWebSocket = () => {
        if (wsRef.current) {
            console.log('WS: Already connected or connecting');
            return;
        }

        let userId = localStorage.getItem('teacher_unique_id');

        if (!userId) {
            const uStr = localStorage.getItem('user_info');
            if (uStr) {
                try {
                    const u = JSON.parse(uStr);
                    userId = u.teacher_unique_id;
                } catch (e) {
                    console.error("Parse user_info failed", e);
                }
            }
        }

        if (!userId) {
            log('无法连接WS: 未获取到用户ID', 'error');
            return;
        }

        const url = `ws://47.100.126.194:5000/ws/${userId}`;
        log(`连接WebSocket: ${url}`, 'info');

        const ws = new WebSocket(url);
        wsRef.current = ws;

        ws.onopen = () => {
            if (ws !== wsRef.current) return;
            log('WebSocket 已连接', 'success');
            // Request Mic Permission preemptively
            requestMicPermission();
        };

        ws.onmessage = (event) => {
            if (ws !== wsRef.current) return;
            try {
                if (event.data === 'pong') return;
                const msg = JSON.parse(event.data);
                handleWsMessage(msg);
            } catch (e) {
                console.error("WS Parse error", e);
            }
        };

        ws.onclose = () => {
            if (ws !== wsRef.current) return;
            log('WebSocket 已断开', 'error');
        };

        ws.onerror = (e) => {
            if (ws !== wsRef.current) return;
            // Only log genuine errors from the active socket
            log('WebSocket 错误', 'error');
        };
    };

    const handleWsMessage = (msg: any) => {
        switch (msg.type) {
            case 'room_created':
            case '6':
                log(`房间创建成功 ID: ${msg.room_id}`, 'success');
                const newInfo = {
                    room_id: msg.room_id,
                    whip_url: msg.whip_url,
                    whep_url: msg.whep_url,
                    stream_name: msg.stream_name || msg.room_id,
                    group_id: msg.group_id || groupId || ""
                };
                setRoomInfo(newInfo);
                if (msg.whep_url) {
                    startPullStream(msg.whep_url, newInfo);
                }
                break;
            case 'srs_answer':
                handleSrsAnswer(msg);
                break;
            case 'voice_speaking':
                if (msg.is_speaking) {
                    setSpeakingUser(msg.user_id);
                } else if (speakingUser === msg.user_id) {
                    setSpeakingUser(null);
                }
                break;
            case 'temp_room_closed':
                log('房间已解散', 'warn');
                setRoomInfo(null);
                stopPublishing(); // Stop if speaking
                break;
            default:
                // console.log("Unhandled Msg", msg);
                break;
        }
    };

    const requestMicPermission = async () => {
        try {
            const stream = await navigator.mediaDevices.getUserMedia({ audio: true });
            publishStreamRef.current = stream; // Keep it open for reuse
            log('麦克风权限获取成功', 'success');
            setIntercomEnabled(true);
        } catch (e: any) {
            log(`无法获取麦克风: ${e.message}`, 'error');
        }
    };

    // ================== Pull Stream (Listen) ==================
    const startPullStream = async (url: string, rInfo: RoomInfo) => {
        if (!wsRef.current) return;

        try {
            if (subscribePcRef.current) subscribePcRef.current.close();

            const pc = new RTCPeerConnection({ iceServers: [{ urls: 'stun:stun.l.google.com:19302' }] });
            subscribePcRef.current = pc;

            pc.ontrack = (ev) => {
                log('收到音频流', 'success');
                const audio = document.createElement('audio');
                audio.srcObject = ev.streams[0];
                audio.autoplay = true;
                // document.body.appendChild(audio);
            };

            // Audio-only offer
            pc.addTransceiver('audio', { direction: 'recvonly' });

            const offer = await pc.createOffer();
            await pc.setLocalDescription(offer);

            const streamName = rInfo.stream_name || rInfo.room_id;
            const msg = {
                type: "srs_play_offer",
                sdp: pc.localDescription?.sdp,
                stream_name: streamName,
                room_id: rInfo.room_id,
                group_id: rInfo.group_id
            };
            wsRef.current.send(JSON.stringify(msg));
            log('发送拉流请求...', 'info');

        } catch (e: any) {
            log(`拉流失败: ${e.message}`, 'error');
        }
    };

    // ================== Push Stream (Talk) ==================
    const startPublishing = async () => {
        if (!roomInfo || !wsRef.current) {
            log('无法推流: 房间未就绪或WS未连接', 'error');
            return;
        }

        // Prevent multiple starts
        if (isTalkingRef.current) return;
        isTalkingRef.current = true;

        try {
            log('开始推流...', 'info');
            setIsPublishing(true);

            if (publishPcRef.current) publishPcRef.current.close();
            const pc = new RTCPeerConnection({ iceServers: [{ urls: 'stun:stun.l.google.com:19302' }] });
            publishPcRef.current = pc;

            // Reuse stream if possible
            let stream = publishStreamRef.current;
            if (!stream || stream.getTracks().some(t => t.readyState === 'ended')) {
                stream = await navigator.mediaDevices.getUserMedia({ audio: true });
                publishStreamRef.current = stream;
            }

            // Check if user released button during await
            if (!isTalkingRef.current) {
                log('推流已取消 (释放太快)', 'warn');
                pc.close();
                publishPcRef.current = null;
                setIsPublishing(false);
                return;
            }

            stream.getTracks().forEach(track => pc.addTrack(track, stream));

            const offer = await pc.createOffer();
            await pc.setLocalDescription(offer);

            // Check again
            if (!isTalkingRef.current) {
                log('推流已取消', 'warn');
                // Send nothing? or cleanup?
                // If we don't send offer, server state is fine.
                // Just cleanup PC
                pc.close();
                publishPcRef.current = null;
                setIsPublishing(false);
                return;
            }

            const streamName = roomInfo.stream_name || roomInfo.room_id;
            const msg = {
                type: "srs_publish_offer",
                sdp: pc.localDescription?.sdp,
                stream_name: streamName,
                room_id: roomInfo.room_id,
                group_id: roomInfo.group_id
            };
            wsRef.current.send(JSON.stringify(msg));

        } catch (e: any) {
            log(`推流失败: ${e.message}`, 'error');
            setIsPublishing(false);
            isTalkingRef.current = false;
        }
    };

    const stopPublishing = () => {
        if (!isTalkingRef.current) return; // Ignore redundant stops

        log('停止推流', 'info');
        setIsPublishing(false);
        isTalkingRef.current = false;

        if (publishPcRef.current) {
            publishPcRef.current.close();
            publishPcRef.current = null;
        }
        // Keep stream for reuse? Or stop? 
        // Logic in HTML says keep it.
    };

    const handleSrsAnswer = async (msg: any) => {
        try {
            if (msg.action === 'publish' && publishPcRef.current) {
                await publishPcRef.current.setRemoteDescription({ type: 'answer', sdp: msg.sdp });
                log('推流连接建立成功', 'success');
            } else if (msg.action === 'play' && subscribePcRef.current) {
                await subscribePcRef.current.setRemoteDescription({ type: 'answer', sdp: msg.sdp });
                log('拉流连接建立成功', 'success');
            }
        } catch (e: any) {
            log(`SRS Answer Error: ${e.message}`, 'error');
        }
    };

    // Toggle for debug logs
    const [showLogs, setShowLogs] = useState(false);

    return (
        <div className="h-screen w-screen bg-[#f0f2f5] text-slate-800 flex flex-col items-center justify-center relative overflow-hidden font-sans">
            {/* Background Decorations */}
            <div className="absolute top-0 left-0 w-full h-64 bg-gradient-to-b from-blue-100/50 to-transparent pointer-events-none"></div>
            <div className="absolute -top-24 -right-24 w-64 h-64 bg-blue-400/10 rounded-full blur-3xl pointer-events-none"></div>
            <div className="absolute bottom-0 -left-24 w-80 h-80 bg-indigo-400/10 rounded-full blur-3xl pointer-events-none"></div>

            {/* Main Card */}
            <div className="w-full max-w-lg bg-white/80 backdrop-blur-xl rounded-3xl shadow-2xl border border-white/60 flex flex-col overflow-hidden h-[90vh] max-h-[800px] ring-1 ring-black/5 relative z-10 transition-all">

                {/* Header */}
                <div className="px-6 py-4 bg-white/50 backdrop-blur-md border-b border-gray-100 flex justify-between items-center relative z-20">
                    <div className="flex items-center gap-3">
                        <div className={`w-2.5 h-2.5 rounded-full ${roomInfo ? 'bg-green-500 animate-pulse' : 'bg-red-400'}`}></div>
                        <div>
                            <h1 className="text-lg font-bold text-slate-800 tracking-tight">班级对讲</h1>
                            <div
                                className="flex items-center gap-1.5 cursor-pointer group"
                                onClick={() => setShowLogs(!showLogs)}
                            >
                                <p className="text-xs text-slate-500 font-medium group-hover:text-blue-600 transition-colors">
                                    {roomInfo ? '正在通话中' : '正在连接...'}
                                </p>
                                <Activity size={10} className="text-slate-400 group-hover:text-blue-600 transition-colors" />
                            </div>
                        </div>
                    </div>
                    <div className="flex gap-2">
                        {/* Future Volume Control - currently disabled/hidden or separate */}
                        <button
                            className="p-2 hover:bg-gray-100 text-gray-400 hover:text-gray-600 rounded-xl transition-colors"
                            title="静音 (暂未实装)"
                        >
                            <Volume2 size={18} />
                        </button>

                        <button
                            onClick={() => getCurrentWindow().close()}
                            className="p-2 hover:bg-red-50 text-slate-400 hover:text-red-500 rounded-xl transition-colors"
                        >
                            <X size={20} />
                        </button>
                    </div>
                </div>

                {/* Participants Area */}
                <div className="flex-1 overflow-y-auto p-6 scrollbar-hide relative z-10">
                    {members.length > 0 ? (
                        <div className="grid grid-cols-4 gap-4 pb-24"> {/* Added padding bottom for floating controls */}
                            {members.map(m => {
                                const isSpeaking = speakingUser == String(m.id);
                                return (
                                    <div key={m.id} className="flex flex-col items-center group">
                                        <div className="relative">
                                            <div className={`w-14 h-14 rounded-2xl flex items-center justify-center text-lg font-bold text-white shadow-sm transition-all duration-300
                                                ${m.avatar ? 'bg-gray-100' : 'bg-gradient-to-br from-blue-400 to-indigo-500'}
                                                ${isSpeaking ? 'ring-4 ring-green-400/50 scale-105 shadow-green-200' : 'group-hover:scale-105 group-hover:shadow-md'}
                                            `}>
                                                {m.avatar ? (
                                                    <img src={m.avatar} className="w-full h-full object-cover rounded-2xl" />
                                                ) : (
                                                    m.name[0]
                                                )}

                                                {/* Status Indicator */}
                                                <div className={`absolute -bottom-1 -right-1 w-4 h-4 border-2 border-white rounded-full 
                                                    ${isSpeaking ? 'bg-green-500 animate-bounce' : 'bg-gray-300'}
                                                `}></div>
                                            </div>

                                            {/* Sound Wave Animation if speaking */}
                                            {isSpeaking && (
                                                <div className="absolute -inset-2 rounded-3xl border-2 border-green-400/30 animate-ping pointer-events-none"></div>
                                            )}
                                        </div>
                                        <span className={`mt-2 text-xs font-medium truncate max-w-[80px] text-center px-1 py-0.5 rounded
                                            ${isSpeaking ? 'text-green-600 bg-green-50' : 'text-slate-600'}
                                        `}>
                                            {m.name}
                                        </span>
                                    </div>
                                );
                            })}
                        </div>
                    ) : (
                        <div className="flex flex-col items-center justify-center h-full text-slate-400 gap-2">
                            <div className="w-16 h-16 rounded-full bg-slate-100 flex items-center justify-center mb-2">
                                <MicOff size={24} className="opacity-50" />
                            </div>
                            <p className="text-sm">暂无成员加入</p>
                        </div>
                    )}
                </div>

                {/* Bottom Sheet Debug Logs */}
                <div
                    className={`absolute bottom-0 left-0 right-0 bg-white/95 backdrop-blur-xl border-t border-gray-200 shadow-[0_-10px_40px_rgba(0,0,0,0.1)] transition-all duration-500 ease-in-out z-30 flex flex-col
                        ${showLogs ? 'h-[60%] opacity-100 translate-y-0' : 'h-0 opacity-0 translate-y-full pointer-events-none'}
                    `}
                >
                    <div className="flex items-center justify-between p-3 border-b border-gray-100 bg-gray-50/50">
                        <div className="flex items-center gap-2 text-slate-600 font-bold text-xs uppercase tracking-wider">
                            <Terminal size={14} className="text-blue-500" />
                            <span>系统日志</span>
                        </div>
                        <button onClick={() => setShowLogs(false)} className="p-1 hover:bg-gray-200 rounded text-gray-400">
                            <X size={14} />
                        </button>
                    </div>
                    <div className="flex-1 overflow-y-auto p-4 font-mono text-[10px] space-y-1.5">
                        {statusLogs.map((l, i) => (
                            <div key={i} className={`flex gap-2 ${l.type === 'error' ? 'text-red-500' :
                                l.type === 'success' ? 'text-emerald-600' :
                                    l.type === 'warn' ? 'text-amber-500' : 'text-slate-500'
                                }`}>
                                <span className="text-slate-300 select-none">[{l.msg.split(' ')[0]}]</span>
                                <span className="break-all">{l.msg.split(' ').slice(1).join(' ')}</span>
                            </div>
                        ))}
                        <div ref={statusLogRef}></div>
                    </div>
                </div>

                {/* Footer / Controls */}
                <div className="p-8 bg-white/60 backdrop-blur-md border-t border-white/50 flex flex-col items-center justify-center relative z-20">
                    {/* Ripple Effect Background when talking */}
                    {isPublishing && (
                        <>
                            <div className="absolute top-1/2 left-1/2 -translate-x-1/2 -translate-y-1/2 w-48 h-48 bg-blue-400/10 rounded-full animate-ping pointer-events-none"></div>
                            <div className="absolute top-1/2 left-1/2 -translate-x-1/2 -translate-y-1/2 w-64 h-64 bg-blue-400/5 rounded-full animate-ping animation-delay-500 pointer-events-none"></div>
                        </>
                    )}

                    <button
                        disabled={!intercomEnabled || !roomInfo}
                        onMouseDown={startPublishing}
                        onMouseUp={stopPublishing}
                        onMouseLeave={stopPublishing}
                        onTouchStart={startPublishing}
                        onTouchEnd={stopPublishing}
                        className={`
                            group relative w-24 h-24 rounded-full flex items-center justify-center
                            shadow-[0_10px_30px_-5px_rgba(59,130,246,0.3)] transition-all duration-200
                            ${!intercomEnabled || !roomInfo ? 'bg-slate-100 text-slate-300 cursor-not-allowed border-4 border-slate-200' :
                                isPublishing ? 'scale-110 shadow-blue-500/50 translate-y-1' :
                                    'hover:scale-105 hover:shadow-blue-500/40 hover:-translate-y-0.5 cursor-pointer'}
                        `}
                    >
                        {/* Button Background Gradient */}
                        <div className={`absolute inset-0 rounded-full transition-all duration-300 ${!intercomEnabled || !roomInfo ? 'bg-slate-100' :
                            isPublishing ? 'bg-gradient-to-br from-blue-500 to-indigo-600' :
                                'bg-gradient-to-br from-blue-400 to-indigo-500 group-hover:from-blue-500 group-hover:to-indigo-600'
                            }`}></div>

                        {/* Icon */}
                        <div className="relative z-10 text-white">
                            {isPublishing ? (
                                <Mic size={40} className="animate-pulse" />
                            ) : (
                                <Mic size={32} className={!intercomEnabled || !roomInfo ? 'text-slate-300' : 'drop-shadow-md'} />
                            )}
                        </div>
                    </button>

                    <p className={`mt-4 text-xs font-semibold transition-colors ${isPublishing ? 'text-blue-600' : 'text-slate-400'}`}>
                        {isPublishing ? "正在讲话..." : (intercomEnabled ? "按住说话" : "准备中...")}
                    </p>
                </div>
            </div>
        </div>
    );
};

export default IntercomWindow;
