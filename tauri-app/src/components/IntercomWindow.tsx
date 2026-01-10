import { useEffect, useRef, useState } from 'react';
import { useParams } from 'react-router-dom';
import { invoke } from '@tauri-apps/api/core';
import { Mic, MicOff, Volume2, X } from 'lucide-react';

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
                        id: u.teacher_unique_id,
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
                    .then(resStr => {
                        const res = JSON.parse(resStr);
                        if (res.data?.members) {
                            const mList = Array.isArray(res.data.members) ? res.data.members : [];
                            setMembers(mList.map((m: any) => ({
                                id: m.user_id || m.id || m.Member_Account,
                                name: m.user_name || m.name || m.student_name || "未知",
                                avatar: m.face_url || m.avatar
                            })));

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
                invoke<string>('fetch_temp_room', { groupId })
                    .then(resStr => {
                        const res = JSON.parse(resStr);
                        // Logic here depends on API output. Assuming standard response format.
                        // Usually it returns a list of rooms or a single room object.
                        // From logs, C++ looks at data["temp_room"] or root object.

                        // If res.code != 200, log error.
                        // Let's assume standard {code, data: { ... }}
                        if (res.data && res.data[0]) { // response key "rooms"? or just array?
                            // TODO: refine parsing based on actual response if needed
                            // For now rely on WS 'room_created' mostly if this fails
                        }
                    })
                    .catch(err => console.error("Fetch temp room error", err));

            } catch (e) {
                log(`初始化数据失败: ${e}`, 'error');
            }

            // 3. Connect WebSocket
            connectWebSocket();
        };

        init();

        return () => {
            // Cleanup
            if (wsRef.current) wsRef.current.close();
            if (publishPcRef.current) publishPcRef.current.close();
            if (subscribePcRef.current) subscribePcRef.current.close();
            if (publishStreamRef.current) publishStreamRef.current.getTracks().forEach(t => t.stop());
            // subscribeStreamRef tracks are remote, handled by PC close usually
        };
    }, [groupId]);

    const connectWebSocket = () => {
        const uStr = localStorage.getItem('user_info');
        if (!uStr) {
            log('无法连接WS: 未登录', 'error');
            return;
        }
        const u = JSON.parse(uStr);
        const userId = u.teacher_unique_id;

        const url = `ws://47.100.126.194:5000/ws/${userId}`;
        log(`连接WebSocket: ${url}`, 'info');

        const ws = new WebSocket(url);
        wsRef.current = ws;

        ws.onopen = () => {
            log('WebSocket 已连接', 'success');
            // Request Mic Permission preemptively
            requestMicPermission();
        };

        ws.onmessage = (event) => {
            try {
                if (event.data === 'pong') return;
                const msg = JSON.parse(event.data);
                handleWsMessage(msg);
            } catch (e) {
                console.error("WS Parse error", e);
            }
        };

        ws.onclose = () => {
            log('WebSocket 已断开', 'error');
        };

        ws.onerror = (e) => {
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

            stream.getTracks().forEach(track => pc.addTrack(track, stream));

            const offer = await pc.createOffer();
            await pc.setLocalDescription(offer);

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
        }
    };

    const stopPublishing = () => {
        log('停止推流', 'info');
        setIsPublishing(false);
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

    return (
        <div className="h-screen w-screen bg-[#3C3C3C] text-white flex flex-col items-center justify-center p-4">
            {/* Header */}
            <div className="w-full max-w-4xl bg-[#2C2C2C] rounded-t-xl p-4 flex justify-between items-center border-b border-[#4C4C4C]">
                <div className="flex items-center gap-2 font-bold">
                    <span>对讲机</span>
                    <span className="text-xs bg-blue-600 px-2 py-0.5 rounded text-white">{roomInfo ? '在线' : '等待房间...'}</span>
                </div>
                <div className="flex gap-2">
                    <button className="bg-[#4C4C4C] hover:bg-[#5C5C5C] px-3 py-1 rounded text-sm transition-colors">静音</button>
                    <button onClick={() => window.close()} className="hover:bg-red-500/20 p-1 rounded transition-colors"><X size={20} /></button>
                </div>
            </div>

            {/* Participants */}
            <div className="w-full max-w-4xl bg-[#3C3C3C] border-x border-[#4C4C4C] p-6">
                <h3 className="text-sm text-gray-400 mb-4">参与者 ({members.length})</h3>
                <div className="flex gap-4 overflow-x-auto pb-2 scrollbar-hide">
                    {/* Current User First? Or Just List */}
                    {members.map(m => (
                        <div key={m.id} className="flex flex-col items-center gap-2 min-w-[70px]">
                            <div className={`w-16 h-16 rounded-lg flex items-center justify-center text-xl font-bold relative transition-all ${speakingUser == String(m.id) ? 'ring-4 ring-green-500 shadow-lg shadow-green-500/20' : 'ring-2 ring-transparent bg-gradient-to-br from-indigo-500 to-purple-600'}`}>
                                {m.avatar ? <img src={m.avatar} className="w-full h-full object-cover rounded-lg" /> : m.name[0]}
                                {speakingUser == String(m.id) && <div className="absolute -top-1 -right-1 w-3 h-3 bg-green-500 rounded-full border-2 border-[#3C3C3C] animate-pulse" />}
                            </div>
                            <span className="text-xs text-gray-300 truncate max-w-[70px]">{m.name}</span>
                        </div>
                    ))}
                    {members.length === 0 && <div className="text-gray-500 text-sm italic">加载成员中...</div>}
                </div>
            </div>

            {/* Status Log */}
            <div className="w-full max-w-4xl flex-1 bg-[#2C2C2C] border-x border-[#4C4C4C] overflow-hidden flex flex-col">
                <div className="p-2 bg-[#252525] text-xs text-gray-400">状态日志</div>
                <div ref={statusLogRef} className="flex-1 overflow-y-auto p-4 space-y-2 font-mono text-xs">
                    {statusLogs.map((l, i) => (
                        <div key={i} className={`border-l-2 pl-2 ${l.type === 'error' ? 'border-red-500 text-red-400' : l.type === 'success' ? 'border-green-500 text-green-400' : l.type === 'warn' ? 'border-orange-500 text-orange-400' : 'border-blue-500 text-gray-300'}`}>
                            {l.msg}
                        </div>
                    ))}
                </div>
            </div>

            {/* Footer Push-to-Talk */}
            <div className="w-full max-w-4xl bg-[#2C2C2C] rounded-b-xl p-6 flex flex-col items-center gap-4 border-t border-[#4C4C4C]">
                <button
                    disabled={!intercomEnabled}
                    onMouseDown={startPublishing}
                    onMouseUp={stopPublishing}
                    onMouseLeave={stopPublishing}
                    onTouchStart={startPublishing}
                    onTouchEnd={stopPublishing}
                    className={`
                        w-20 h-20 rounded-full flex items-center justify-center text-3xl shadow-xl transition-all select-none
                        ${!intercomEnabled ? 'bg-gray-600 text-gray-400 cursor-not-allowed' :
                            isPublishing ? 'bg-red-500 text-white scale-110 ring-4 ring-red-500/30' : 'bg-blue-600 text-white hover:bg-blue-500 hover:scale-105 active:scale-95'}
                    `}
                >
                    {isPublishing ? <MicOff /> : <Mic />}
                </button>
                <span className="text-xs text-gray-400">
                    {intercomEnabled ? "按住按钮开始对讲，松开按钮停止" : "正在获取麦克风..."}
                </span>
            </div>
        </div>
    );
};

export default IntercomWindow;
