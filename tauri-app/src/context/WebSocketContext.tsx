import React, { createContext, useContext, useEffect, useState, useRef } from 'react';
import { emit, listen } from '@tauri-apps/api/event';
import { getCurrentWindow } from '@tauri-apps/api/window';

interface WebSocketContextType {
    sendMessage: (msg: string) => void;
    isConnected: boolean;
    lastMessage: string | null;
    latestNotifications: any[];
}

const WebSocketContext = createContext<WebSocketContextType | null>(null);

export const WebSocketProvider = ({ children }: { children: React.ReactNode }) => {
    const [isConnected, setIsConnected] = useState(false);
    const [lastMessage, setLastMessage] = useState<string | null>(null);
    const [latestNotifications, setLatestNotifications] = useState<any[]>([]);
    const socketRef = useRef<WebSocket | null>(null);
    const heartbeatTimerRef = useRef<number | null>(null);
    const [isMaster, setIsMaster] = useState(false);

    useEffect(() => {
        const win = getCurrentWindow();
        // Label 'main' acts as the Master who owns the physical connection
        const master = win.label === 'main';
        setIsMaster(master);

        let ws: WebSocket | null = null;
        let reconnectTimer: any = null;

        // --- Slave Logic: Listen to broadcasts from Master ---
        const unlistenMsg = listen<string>('ws-broadcast', (event) => {
            if (event.payload !== 'pong') {
                setLastMessage(event.payload);
                try {
                    const parsed = JSON.parse(event.payload);
                    if (parsed.type === "unread_notifications") {
                        setLatestNotifications(parsed.data || []);
                    }
                } catch (e) {}
                window.dispatchEvent(new CustomEvent('ws-message', { detail: event.payload }));
            }
        });

        const unlistenStatus = listen<boolean>('ws-status-change', (event) => {
            console.log("[WS Slave] Status Change:", event.payload);
            setIsConnected(event.payload);
        });

        if (!master) {
            // Slaves request initial status
            emit('ws-status-request');
        }

        // --- Master Logic: Handle physical connection and proxying ---
        let unlistenRequest: any = null;
        let unlistenStatusReq: any = null;

        if (master) {
            // Master responds to status requests from Slaves
            unlistenStatusReq = listen('ws-status-request', () => {
                const currentStatus = socketRef.current && socketRef.current.readyState === WebSocket.OPEN;
                console.log("[WS Master] Status Request received, responding with:", currentStatus);
                emit('ws-status-change', currentStatus);
            });

            const connect = () => {
                let teacherId = localStorage.getItem('teacher_unique_id');
                const storedUser = localStorage.getItem('user_info');
                if (!teacherId && storedUser) {
                    try {
                        const u = JSON.parse(storedUser);
                        teacherId = u.teacher_unique_id || u.unique_id || u.id || u.user_id || (u.data && u.data.teacher_unique_id) || (u.data && u.data.id);
                    } catch (e) {}
                }

                if (!teacherId) {
                    reconnectTimer = setTimeout(connect, 2000);
                    return;
                }

                const url = `ws://47.100.126.194:5000/ws/${teacherId}`;
                console.log("[WS Master] Connecting to", url);

                ws = new WebSocket(url);

                ws.onopen = () => {
                    console.log("[WS Master] Connected");
                    setIsConnected(true);
                    socketRef.current = ws;
                    emit('ws-status-change', true);

                    if (heartbeatTimerRef.current) clearInterval(heartbeatTimerRef.current);
                    heartbeatTimerRef.current = window.setInterval(() => {
                        if (ws && ws.readyState === WebSocket.OPEN) {
                            ws.send("ping");
                        }
                    }, 5000);
                };

                ws.onclose = () => {
                    console.log("[WS Master] Closed");
                    setIsConnected(false);
                    socketRef.current = null;
                    emit('ws-status-change', false);
                    if (heartbeatTimerRef.current) clearInterval(heartbeatTimerRef.current);
                    reconnectTimer = setTimeout(connect, 3000);
                };

                ws.onerror = (e) => {
                    console.error("[WS Master] Error", e);
                };

                ws.onmessage = (event) => {
                    if (event.data !== 'pong') {
                        setLastMessage(String(event.data));
                        try {
                            const parsed = JSON.parse(event.data);
                            if (parsed.type === "unread_notifications") {
                                setLatestNotifications(parsed.data || []);
                            }
                        } catch (e) {}
                        
                        emit('ws-broadcast', String(event.data));
                        window.dispatchEvent(new CustomEvent('ws-message', { detail: event.data }));
                    }
                };
            };

            connect();

            // Listen for send requests from Slaves
            unlistenRequest = listen<string>('ws-send-request', (event) => {
                if (socketRef.current && socketRef.current.readyState === WebSocket.OPEN) {
                    socketRef.current.send(event.payload);
                } else {
                    console.warn("[WS Master] Cannot proxy message: connection not ready");
                }
            });
        }

        return () => {
            if (master && ws) ws.close();
            if (reconnectTimer) clearTimeout(reconnectTimer);
            if (heartbeatTimerRef.current) clearInterval(heartbeatTimerRef.current);
            unlistenMsg.then(f => f());
            unlistenStatus.then(f => f());
            if (unlistenRequest) unlistenRequest.then((f: any) => f());
        };
    }, []);

    const sendMessage = (msg: string) => {
        if (isMaster) {
            if (socketRef.current && socketRef.current.readyState === WebSocket.OPEN) {
                socketRef.current.send(msg);
            } else {
                console.warn("[WS Master] Socket not open");
            }
        } else {
            // Slaves emit an event that Master listens to
            emit('ws-send-request', msg);
        }
    };

    return (
        <WebSocketContext.Provider value={{ sendMessage, isConnected, lastMessage, latestNotifications }}>
            {children}
        </WebSocketContext.Provider>
    );
};

export const useWebSocket = () => {
    const context = useContext(WebSocketContext);
    if (!context) {
        throw new Error("useWebSocket must be used within a WebSocketProvider");
    }
    return context;
};
