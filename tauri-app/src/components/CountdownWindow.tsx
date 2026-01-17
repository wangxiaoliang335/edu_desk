import CountdownWidget from './CountdownWidget';
import { useState, useEffect, useMemo, useRef } from 'react';
import { getCurrentWindow, LogicalSize } from '@tauri-apps/api/window';
import { WebviewWindow } from '@tauri-apps/api/webviewWindow';
import { invoke } from '@tauri-apps/api/core';
import { listen } from '@tauri-apps/api/event';
import '../styles/transparent.css';

const loadCountdownConfig = () => {
    const saved = localStorage.getItem('countdown_config');
    return saved ? JSON.parse(saved) : {
        title: '距离高考',
        targetDate: new Date(new Date().getFullYear(), 5, 7).toISOString().split('T')[0]
    };
};

export default function CountdownWindow() {
    useEffect(() => console.log('CountdownWindow Mounted'), []); // Debug log
    const [config, setConfig] = useState(loadCountdownConfig);
    const widgetWrapRef = useRef<HTMLDivElement | null>(null);
    const modeParam = useMemo(() => new URLSearchParams(window.location.search).get('mode'), []);
    const windowMode = modeParam === 'full' ? 'full' : 'minimal';

    // Window background handling (minimal uses solid bg, full uses transparent)
    useEffect(() => {
        const root = document.getElementById('root');
        if (windowMode === 'minimal') {
            document.documentElement.classList.add('transparent-window');
            document.body.classList.add('transparent-window');
            if (root) root.classList.add('transparent-window');
            document.documentElement.style.background = 'transparent';
            document.body.style.background = 'transparent';
            if (root) root.style.background = 'transparent';
            document.documentElement.style.width = '100%';
            document.documentElement.style.height = '100%';
            document.body.style.width = '100%';
            document.body.style.height = '100%';
            document.body.style.margin = '0';
            document.body.style.overflow = 'hidden';
            if (root) {
                root.style.width = '100%';
                root.style.height = '100%';
            }
            document.body.classList.remove('bg-paper');
            document.body.classList.remove('bg-background');
        } else {
            document.documentElement.classList.remove('transparent-window');
            document.body.classList.remove('transparent-window');
            if (root) root.classList.remove('transparent-window');
            document.documentElement.style.background = '#f8f6f1';
            document.body.style.background = '#f8f6f1';
            if (root) root.style.background = '#f8f6f1';
            document.documentElement.style.width = 'fit-content';
            document.documentElement.style.height = 'fit-content';
            document.body.style.width = 'fit-content';
            document.body.style.height = 'fit-content';
            document.body.style.margin = '0';
            document.body.style.overflow = 'hidden';
            if (root) {
                root.style.width = 'fit-content';
                root.style.height = 'fit-content';
            }
            document.body.classList.remove('bg-paper');
            document.body.classList.remove('bg-background');
        }

        return () => {
            document.documentElement.classList.remove('transparent-window');
            document.body.classList.remove('transparent-window');
            if (root) root.classList.remove('transparent-window');
            document.documentElement.style.background = '';
            document.body.style.background = '';
            if (root) root.style.background = '';
            document.documentElement.style.width = '';
            document.documentElement.style.height = '';
            document.body.style.width = '';
            document.body.style.height = '';
            document.body.style.margin = '';
            document.body.style.overflow = '';
            if (root) {
                root.style.width = '';
                root.style.height = '';
            }
            document.body.classList.add('bg-paper');
        };
    }, [windowMode]);

    const switchToWindow = async (mode: 'full' | 'minimal') => {
        if (mode === windowMode) return;
        const current = getCurrentWindow();
        const targetLabel = mode === 'full' ? 'countdown_full' : 'countdown_minimal';
        const otherLabel = mode === 'full' ? 'countdown_minimal' : 'countdown_full';
        let target = await WebviewWindow.getByLabel(targetLabel);
        if (target) {
            await target.show();
            await target.setFocus();
        } else {
            await invoke(mode === 'full' ? 'open_countdown_full_window' : 'open_countdown_minimal_window');
            target = await WebviewWindow.getByLabel(targetLabel);
        }
        const other = await WebviewWindow.getByLabel(otherLabel);
        if (other) {
            await other.hide();
        }
        await current.hide();
    };



    useEffect(() => {
        if (windowMode !== 'full') return;
        const window = getCurrentWindow();
        const el = widgetWrapRef.current;
        if (!el) return;

        const resizeToFit = () => {
            const rect = el.getBoundingClientRect();
            const width = Math.ceil(rect.width);
            const height = Math.ceil(rect.height);
            if (width > 0 && height > 0) {
                window.setSize(new LogicalSize(width, height));
                window.setMinSize(new LogicalSize(width, height));
                window.setMaxSize(new LogicalSize(width, height));
            }
        };

        const scheduleResize = () => {
            requestAnimationFrame(() => {
                resizeToFit();
                setTimeout(resizeToFit, 80);
                setTimeout(resizeToFit, 200);
            });
        };

        scheduleResize();
        const observer = new ResizeObserver(resizeToFit);
        observer.observe(el);
        return () => observer.disconnect();
    }, [windowMode]);

    useEffect(() => {
        const unlisten = listen('countdown-config-updated', () => {
            setConfig(loadCountdownConfig());
        });
        return () => {
            unlisten.then((fn) => fn());
        };
    }, []);

    const openEditWindow = async () => {
        await invoke('open_countdown_edit_window');
        await getCurrentWindow().hide();
    };

    const containerClass = windowMode === 'minimal'
        ? "w-full h-full flex items-center justify-center bg-transparent"
        : "w-fit h-fit bg-transparent overflow-hidden flex items-start justify-start pt-0";

    return (
        <div className={containerClass}>
            {/*
               Window is transparent.
               CountdownWidget handles its own rendering.
            */}
            <div
                ref={widgetWrapRef}
                className={windowMode === 'full' ? 'w-fit h-fit' : 'w-full h-full'}
            >
                <CountdownWidget
                    visible={true}
                    config={config}
                    onEdit={windowMode === 'full' ? openEditWindow : () => switchToWindow('full')}
                    isWindowMode={true}
                    mode={windowMode}
                    onRequestModeChange={switchToWindow}
                />
            </div>
        </div>
    );
}
