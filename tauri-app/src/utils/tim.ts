import TencentCloudChat from '@tencentcloud/chat';
import TIMUploadPlugin from 'tim-upload-plugin';
import { getCurrentWindow } from '@tauri-apps/api/window';
import { emit, listen } from '@tauri-apps/api/event';
// import { v4 as uuidv4 } from 'uuid'; // Removed as we use custom generator

// Simple UUID generator if uuid package is missing
const generateUUID = () => {
    return 'xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx'.replace(/[xy]/g, function (c) {
        var r = Math.random() * 16 | 0, v = c === 'x' ? r : (r & 0x3 | 0x8);
        return v.toString(16);
    });
};

// Helper for file handling (Shared/Proxy side usage typically, but useful to have)
const fileToBase64 = (file: File | HTMLInputElement): Promise<{ name: string, type: string, base64: string }> => {
    return new Promise((resolve, reject) => {
        let actualFile: File | null = null;
        if (file instanceof HTMLInputElement) {
            if (file.files && file.files.length > 0) actualFile = file.files[0];
        } else {
            actualFile = file;
        }

        if (!actualFile) {
            reject(new Error("No file selected"));
            return;
        }

        const reader = new FileReader();
        reader.onload = () => {
            const res = reader.result as string;
            // Remove prefix "data:image/png;base64,"
            const base64 = res.split(',')[1];
            resolve({
                name: actualFile!.name,
                type: actualFile!.type,
                base64
            });
        };
        reader.onerror = reject;
        reader.readAsDataURL(actualFile);
    });
}

// Master-side helper to reconstruct Blob from base64
const base64ToBlob = (base64: string, type: string) => {
    const binStr = atob(base64);
    const len = binStr.length;
    const arr = new Uint8Array(len);
    for (let i = 0; i < len; i++) {
        arr[i] = binStr.charCodeAt(i);
    }
    return new Blob([arr], { type });
}

const SDKAppID = 1600111046;
const TIM_EVENT_CHANNEL = 'tim:event';
const TIM_REQUEST_CHANNEL = 'tim:request';
const TIM_RESPONSE_PREFIX = 'tim:response:';

// State Flags
export let isSDKReady = false;
let isMaster = false;
let tim: any = null;

// Determine if we are Master (Main Window)
const win = getCurrentWindow();
isMaster = win.label === 'main';

console.log(`[TIM] Initializing as ${isMaster ? 'MASTER' : 'SLAVE'} in window: ${win.label}`);

// --- MASTER LOGIC ---
if (isMaster) {
    tim = TencentCloudChat.create({ SDKAppID });
    tim.registerPlugin({ 'tim-upload-plugin': TIMUploadPlugin });
    tim.setLogLevel(1);

    // Forward SDK Events to Slaves
    const forwardEvent = (eventName: string, data?: any) => {
        emit(TIM_EVENT_CHANNEL, { event: eventName, data });
    };

    tim.on(TencentCloudChat.EVENT.SDK_READY, () => {
        console.log('[TIM Master] SDK Ready');
        isSDKReady = true;
        forwardEvent(TencentCloudChat.EVENT.SDK_READY);
    });

    tim.on(TencentCloudChat.EVENT.SDK_NOT_READY, () => {
        console.log('[TIM Master] SDK Not Ready');
        isSDKReady = false;
        forwardEvent(TencentCloudChat.EVENT.SDK_NOT_READY);
    });

    tim.on(TencentCloudChat.EVENT.KICKED_OUT, () => {
        console.log('[TIM Master] Kicked Out');
        isSDKReady = false;
        forwardEvent(TencentCloudChat.EVENT.KICKED_OUT);
    });

    tim.on(TencentCloudChat.EVENT.MESSAGE_RECEIVED, (event: any) => {
        console.log('[TIM Master] Message Received, broadcasting...');
        forwardEvent(TencentCloudChat.EVENT.MESSAGE_RECEIVED, event);
    });

    // Listen for Requests from Slaves
    listen(TIM_REQUEST_CHANNEL, async (event: any) => {
        const { id, command, args } = event.payload;
        console.log(`[TIM Master] Received Request: ${command} (${id})`, args);

        let response = { success: false, data: null, error: null };

        try {
            switch (command) {
                case 'login':
                    // Slaves shouldn't really call login, but if they do, we handle it or ignore
                    // Ideally App.tsx calls login directly on Master
                    response.success = true;
                    break;
                case 'getGroupList':
                    const groupRes = await tim.getGroupList();
                    response.success = true;
                    response.data = groupRes.data.groupList;
                    break;
                case 'getGroupMemberList':
                    const memberRes = await tim.getGroupMemberList(args[0]);
                    response.success = true;
                    response.data = memberRes.data.memberList;
                    break;
                case 'getGroupProfile':
                    const profileRes = await tim.getGroupProfile(args[0]);
                    response.success = true;
                    response.data = profileRes.data;
                    break;
                case 'addGroupMember':
                    const addMemRes = await tim.addGroupMember(args[0]);
                    response.success = true;
                    response.data = addMemRes.data;
                    break;
                case 'getMessageList':
                    const msgRes = await tim.getMessageList(args[0]);
                    response.success = true;
                    response.data = msgRes.data.messageList;
                    break;
                case 'sendMessage':
                    // args: [to, text, type]
                    const [to, text, type] = args;
                    const msg = tim.createTextMessage({
                        to,
                        conversationType: type === 'GROUP' ? TencentCloudChat.TYPES.CONV_GROUP : TencentCloudChat.TYPES.CONV_C2C,
                        payload: { text }
                    });
                    const sendRes = await tim.sendMessage(msg);
                    response.success = true;
                    response.data = sendRes.data.message;
                    break;
                case 'sendImageMessage':
                    {
                        const [to, fileData, type] = args;
                        const blob = base64ToBlob(fileData.base64, fileData.type);
                        const file = new File([blob], fileData.name, { type: fileData.type });

                        const msg = tim.createImageMessage({
                            to,
                            conversationType: type === 'GROUP' ? TencentCloudChat.TYPES.CONV_GROUP : TencentCloudChat.TYPES.CONV_C2C,
                            payload: { file }
                        });
                        const sendRes = await tim.sendMessage(msg);
                        response.success = true;
                        response.data = sendRes.data.message;
                    }
                    break;
                case 'sendFileMessage':
                    {
                        const [to, fileData, type] = args;
                        const blob = base64ToBlob(fileData.base64, fileData.type);
                        const file = new File([blob], fileData.name, { type: fileData.type });

                        const msg = tim.createFileMessage({
                            to,
                            conversationType: type === 'GROUP' ? TencentCloudChat.TYPES.CONV_GROUP : TencentCloudChat.TYPES.CONV_C2C,
                            payload: { file }
                        });
                        const sendRes = await tim.sendMessage(msg);
                        response.success = true;
                        response.data = sendRes.data.message;
                    }
                    break;
                case 'createGroup':
                    const createRes = await tim.createGroup(args[0]);
                    response.success = true;
                    response.data = createRes.data.group;
                    break;
                // Add other commands as needed...
                case 'dismissGroup':
                    await tim.dismissGroup(args[0]);
                    response.success = true;
                    break;
                case 'quitGroup':
                    await tim.quitGroup(args[0]);
                    response.success = true;
                    break;
                case 'changeGroupOwner':
                    await tim.changeGroupOwner(args[0]);
                    response.success = true;
                    break;
                default:
                    response.error = `Unknown command: ${command}` as any;
            }
        } catch (e: any) {
            console.error(`[TIM Master] Command ${command} failed:`, e);
            response.error = e.message || e;
        }

        emit(`${TIM_RESPONSE_PREFIX}${id}`, response);
    });
}

// --- SLAVE LOGIC (AND SHARED PUBLIC API) ---

// Event Listeners (Shared)
const messageListeners: ((event: any) => void)[] = [];

// Listen for global events (broadcasted by Master or local if Master)
// Note: Even Master listens to the broadcast channel to simplify logic if it emits to itself? 
// Actually Master emits to global, so it receives it too? API says "emit to all windows". 
// Let's assume Master handles local events via direct callbacks above, BUT for consistent API usage, 
// we should probably listen to the event channel in both modes if we want uniform behavior.
// However, Master already added 'forwardEvent' which emits. 
// Let's add a listener for everyone to handle public subscription.

listen(TIM_EVENT_CHANNEL, (event: any) => {
    const { event: eventName, data } = event.payload;
    // Update local state based on broadcast
    if (eventName === TencentCloudChat.EVENT.SDK_READY) isSDKReady = true;
    if (eventName === TencentCloudChat.EVENT.SDK_NOT_READY) isSDKReady = false;
    if (eventName === TencentCloudChat.EVENT.KICKED_OUT) isSDKReady = false;

    if (eventName === TencentCloudChat.EVENT.MESSAGE_RECEIVED) {
        messageListeners.forEach(l => l(data));
    }
});


// --- Public API Functions (Facade) ---

const sendRequest = (command: string, ...args: any[]): Promise<any> => {
    return new Promise((resolve, reject) => {
        if (isMaster && command !== 'login') {
            // If we are Master, we COULD call directly, but for 'login' specifically we want special handling.
            // For other commands, we could short-circuit, but using the event loop ensures consistent async behavior.
            // However, reusing the switch case above is cleaner. 
            // To avoid code duplication, let's actually just use the IPC loop even for Master? 
            // Or better: Master calls directly.
            // Refactoring: Let's keep it simple. If Master, we need to call logic directly. 
            // Writing a separate execution function would be best.
            executeMasterCommand(command, args).then(resolve).catch(reject);
            return;
        }

        const id = generateUUID();
        const responseChannel = `${TIM_RESPONSE_PREFIX}${id}`;

        const unlisten = listen(responseChannel, (event: any) => {
            const { success, data, error } = event.payload;
            unlisten.then(f => f()); // Clean up listener
            if (success) resolve(data);
            else reject(error);
        });

        emit(TIM_REQUEST_CHANNEL, { id, command, args });
    });
};

// Helper for Master to execute directly (optimization)
async function executeMasterCommand(command: string, args: any[]) {
    if (!tim) throw new Error("TIM not initialized on Master");
    // This duplicates the switch case, but avoids IPC overhead on Master.
    // For now, let's duplicate the switch logic or extract it.
    // Extraction:
    switch (command) {
        case 'getGroupList': return (await tim.getGroupList()).data.groupList;
        case 'getGroupMemberList': return (await tim.getGroupMemberList(args[0])).data.memberList;
        case 'getMessageList': return (await tim.getMessageList(args[0])).data.messageList;
        case 'sendMessage':
            const [to, text, type] = args;
            const msg = tim.createTextMessage({
                to,
                conversationType: type === 'GROUP' ? TencentCloudChat.TYPES.CONV_GROUP : TencentCloudChat.TYPES.CONV_C2C,
                payload: { text }
            });
            return (await tim.sendMessage(msg)).data.message;
        case 'createGroup': return (await tim.createGroup(args[0])).data.group;
        case 'dismissGroup': return await tim.dismissGroup(args[0]);
        case 'quitGroup': return await tim.quitGroup(args[0]);
        case 'changeGroupOwner': return await tim.changeGroupOwner(args[0]);

        default: throw new Error(`Unknown command: ${command}`);
    }
}


export const addMessageListener = (listener: (event: any) => void) => {
    messageListeners.push(listener);
    return () => {
        const idx = messageListeners.indexOf(listener);
        if (idx > -1) messageListeners.splice(idx, 1);
    };
};

export const loginTIM = async (userID: string, userSig: string) => {
    if (isMaster) {
        // Master performs actual login
        if (isSDKReady) return true;
        try {
            await tim.login({ userID, userSig });

            // Wait for SDK Ready
            return new Promise((resolve) => {
                if (isSDKReady) resolve(true);
                const onReady = () => {
                    tim.off(TencentCloudChat.EVENT.SDK_READY, onReady);
                    resolve(true);
                };
                tim.on(TencentCloudChat.EVENT.SDK_READY, onReady);
                // Timeout fallback
                setTimeout(() => {
                    tim.off(TencentCloudChat.EVENT.SDK_READY, onReady);
                    resolve(false); // or true if we want to be optimistic
                }, 5000);
            });
        } catch (e) {
            console.error("TIM Login Failed:", e);
            return false;
        }
    } else {
        // Slave just pretends to login or requests status
        // In this architecture, Slave assumes Master is handling it.
        // We can check if Master is ready.
        // For now, return true to allow UI to proceed.
        console.log("[TIM Slave] Login called, assuming Master is handling connection.");
        // Optional: Ping Master for status?
        isSDKReady = true; // Optimistic
        return true;
    }
};

export const logoutTIM = async () => {
    if (isMaster) {
        await tim.logout();
    } else {
        console.log("[TIM Slave] Logout ignored (managed by Master)");
    }
};

// --- Proxy Functions ---

export const getTIMGroups = async () => {
    // Slave or Master
    try {
        const res = await sendRequest('getGroupList');
        return res || [];
    } catch (e) {
        console.error("getTIMGroups failed:", e);
        return [];
    }
};

// Keep cache export for compatibility, though IPC reduces need for manual cache syncing between windows
// unless we want to avoid IPC calls.
// Keep cache export for compatibility
export const setCachedTIMGroups = (_groups: any[]) => { /* No-op or update local */ };

export const invalidateTIMGroupsCache = () => {
    // In new architecture, we rely on Master or fresh fetches. 
    // This can be a signal to Master to clear cache if it had one, or just a no-op client side.
    console.log('[TIM Slave] invalidateTIMGroupsCache called (no-op in new arch)');
};

export const addGroupMember = async (groupID: string, userIDList: string[]) => {
    try {
        return await sendRequest('addGroupMember', { groupID, userIDList });
    } catch (e) { throw e; }
};

// Re-implementing correctly:
// Override the export to match original signature
// tim.getGroupMemberList takes {groupID, count, offset}
const getGroupMemberListExport = async (groupID: string) => {
    try {
        return await sendRequest('getGroupMemberList', { groupID, count: 100, offset: 0 });
    } catch (e) { return []; }
}
export { getGroupMemberListExport as getGroupMemberList };


// Helper to delay
const delay = (ms: number) => new Promise(resolve => setTimeout(resolve, ms));

export const getGroupProfile = async (groupID: string) => {
    try {
        return await sendRequest('getGroupProfile', { groupID }); // Pass object
    } catch (e) { return null; }
};

export const checkGroupsExist = async (groupIDs: string[]): Promise<string[]> => {
    // This logic runs on both Master and Slave via the facade
    // However, for efficiency, we should probably implementations move loop to Master if possible?
    // But keeping it here allows reuse of existing code structure.

    if (groupIDs.length === 0) return [];

    const existingGroups: string[] = [];

    // Check groups sequentially with throttling to avoid rate limits
    // Note: We are doing this client-side (or proxy-side). 
    // If Slave, each check is an IPC call -> Master -> SDK.
    for (const groupID of groupIDs) {
        try {
            // We need to use sendRequest manually or expose getGroupProfile
            const res = await sendRequest('getGroupProfile', { groupID });

            // If we get here without error, the group exists
            if (res && res.group && res.group.groupID) {
                existingGroups.push(res.group.groupID);
            }
        } catch (error: any) {
            // We need to parse error from IPC response
            const errCode = error?.code || 0;

            // Error code 10010 means group doesn't exist - this is expected
            // Error code 10007 means "only group member can get group info" - group EXISTS but we're not a member
            if (errCode === 10007) {
                console.log(`TIM checkGroupExist for ${groupID}: exists (not a member, code 10007)`);
                existingGroups.push(groupID);
            } else if (errCode === 2996) {
                // Rate limit - wait and retry
                await delay(500);
                try {
                    const retryRes = await sendRequest('getGroupProfile', { groupID });
                    if (retryRes && retryRes.group) existingGroups.push(retryRes.group.groupID);
                } catch (retryError: any) {
                    if (retryError?.code === 10007) existingGroups.push(groupID);
                }
            }
        }
        // Throttle
        await delay(100);
    }

    console.log('TIM checkGroupsExist: found', existingGroups.length, 'existing groups out of', groupIDs.length);
    return existingGroups;
};

export const getMessageList = async (groupID: string) => {
    try {
        return await sendRequest('getMessageList', { conversationID: `GROUP${groupID}` });
    } catch (e) { return []; }
};

export const sendMessage = async (to: string, text: string, type: 'C2C' | 'GROUP' = 'GROUP') => {
    try {
        return await sendRequest('sendMessage', to, text, type);
    } catch (e) { return null; }
};

// Helpers moved to top

export const sendImageMessage = async (to: string, file: HTMLInputElement | File, type: 'C2C' | 'GROUP' = 'GROUP') => {
    try {
        const fileData = await fileToBase64(file);
        return await sendRequest('sendImageMessage', to, fileData, type);
    } catch (e) { console.error(e); return null; }
};

export const sendFileMessage = async (to: string, file: HTMLInputElement | File, type: 'C2C' | 'GROUP' = 'GROUP') => {
    try {
        const fileData = await fileToBase64(file);
        return await sendRequest('sendFileMessage', to, fileData, type);
    } catch (e) { console.error(e); return null; }
};

// ... Add other exports as needed matching original signature ...
export const createGroup = async (payload: any) => {
    // Simplified proxy
    // Original createGroup took (name, type, members)
    // We need to match that signature or update implementation
    // Let's match original src/utils/tim.ts signature roughly?
    // Actually the usage in App.tsx might look different.
    // Let's implement generic createGroup proxy
    try {
        return await sendRequest('createGroup', payload);
    } catch (e) { throw e; }
};

// Re-implement simplified helpers from original tim.ts
export const dismissGroup = async (groupID: string) => sendRequest('dismissGroup', groupID);
export const quitGroup = async (groupID: string) => sendRequest('quitGroup', groupID);
export const changeGroupOwner = async (groupID: string, newOwnerID: string) =>
    sendRequest('changeGroupOwner', { groupID, newOwnerID });

