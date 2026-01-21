import React, { createContext, useContext, useEffect, useState, useRef } from 'react';

// Keep this type simple for now

interface WebSocketContextType {
    sendMessage: (msg: string) => void;
    isConnected: boolean;
    lastMessage: string | null;
}

const WebSocketContext = createContext<WebSocketContextType | null>(null);

export const WebSocketProvider = ({ children }: { children: React.ReactNode }) => {
    const [isConnected, setIsConnected] = useState(false);
    const [lastMessage, setLastMessage] = useState<string | null>(null);
    const socketRef = useRef<WebSocket | null>(null);
    const heartbeatTimerRef = useRef<number | null>(null);

    // We need the teacher_unique_id to connect. 
    // For now, we'll try to get it from localStorage which we should populate on login.
    // If not found, we might need to listen for login success. 
    // But since this provider wraps the app, it might mount before login.
    // Let's retry connection if ID is missing or use a dynamic connect method?
    // A simpler approach for this app structure: Auto-connect when ID is available locally.

    // Check for ID periodically or listen to an event? 
    // Let's assume the ID is stored in localStorage 'teacher_unique_id' upon login.

    useEffect(() => {
        let ws: WebSocket | null = null;
        let reconnectTimer: any = null;

        const connect = () => {
            // Retrieve ID from storage
            // 1. Try explicit 'teacher_unique_id' first (most reliable)
            let teacherId = localStorage.getItem('teacher_unique_id');
            const storedUser = localStorage.getItem('user_info');

            // 2. Fallback to parsing user_info if needed
            if (!teacherId && storedUser) {
                try {
                    const u = JSON.parse(storedUser);
                    console.log("[WebSocketContext] User Info parsed:", u);
                    // Try to find ANY valid ID
                    teacherId = u.teacher_unique_id || u.unique_id || u.id || u.user_id || (u.data && u.data.teacher_unique_id) || (u.data && u.data.id);
                    console.log("[WebSocketContext] Got Teacher ID from JSON:", teacherId);
                } catch (e) {
                    console.error("[WebSocketContext] JSON Parse Error:", e);
                }
            }

            if (!teacherId) {
                // Not logged in or no ID, retry later
                console.log("[WebSocketContext] No Teacher ID, retrying in 2s...");
                reconnectTimer = setTimeout(connect, 2000);
                return;
            }

            const url = `ws://47.100.126.194:5000/ws/${teacherId}`;
            console.log("WS: Connecting to", url);

            ws = new WebSocket(url);

            ws.onopen = () => {
                console.log("WS: Connected");
                setIsConnected(true);
                socketRef.current = ws;

                // Start Heartbeat
                if (heartbeatTimerRef.current) clearInterval(heartbeatTimerRef.current);
                heartbeatTimerRef.current = window.setInterval(() => {
                    if (ws && ws.readyState === WebSocket.OPEN) {
                        ws.send("ping");
                    }
                }, 5000);
            };

            ws.onclose = () => {
                console.log("WS: Closed");
                setIsConnected(false);
                socketRef.current = null;
                if (heartbeatTimerRef.current) clearInterval(heartbeatTimerRef.current);
                reconnectTimer = setTimeout(connect, 3000); // Reconnect
            };

            ws.onerror = (e) => {
                console.error("WS: Error", e);
            };

            ws.onmessage = (event) => {
                if (event.data !== 'pong') {
                    console.log("[MonitorFlow] Raw WS Message Received:", event.data);
                    setLastMessage(String(event.data));
                    window.dispatchEvent(new CustomEvent('ws-message', { detail: event.data }));
                }
            };
        };

        connect();

        return () => {
            if (ws) ws.close();
            if (reconnectTimer) clearTimeout(reconnectTimer);
            if (heartbeatTimerRef.current) clearInterval(heartbeatTimerRef.current);
        };
    }, []); // Empty dependency array means it runs once, but we assume re-mounting or we could add dependency if user changes

    const sendMessage = (msg: string) => {
        if (socketRef.current && socketRef.current.readyState === WebSocket.OPEN) {
            console.log("[MonitorFlow] Raw WS Message Sending:", msg);
            socketRef.current.send(msg);
        } else {
            console.warn("[MonitorFlow] WS: Cannot send, socket not open or current is null. ReadyState:", socketRef.current?.readyState);
        }
    };

    return (
        <WebSocketContext.Provider value={{ sendMessage, isConnected, lastMessage }}>
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
