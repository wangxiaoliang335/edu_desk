import TencentCloudChat from '@tencentcloud/chat';
import TIMUploadPlugin from 'tim-upload-plugin';

const SDKAppID = 1600111046;

export const tim = TencentCloudChat.create({
    SDKAppID
});

// Register Upload Plugin
tim.registerPlugin({ 'tim-upload-plugin': TIMUploadPlugin });

// Set log level
tim.setLogLevel(1);

export type MessageListener = (event: any) => void;
const messageListeners: MessageListener[] = [];

export const addMessageListener = (listener: MessageListener) => {
    messageListeners.push(listener);
    console.log("TIM: Listener added. Total:", messageListeners.length);
    return () => {
        const index = messageListeners.indexOf(listener);
        if (index > -1) {
            messageListeners.splice(index, 1);
            console.log("TIM: Listener removed. Total:", messageListeners.length);
        }
    };
};

tim.on(TencentCloudChat.EVENT.MESSAGE_RECEIVED, (event: any) => {
    console.log('TIM Event: MESSAGE_RECEIVED', event);
    messageListeners.forEach(listener => listener(event));
});

export let isSDKReady = false;

tim.on(TencentCloudChat.EVENT.SDK_READY, () => {
    console.log('TIM Event: SDK_READY');
    isSDKReady = true;
});

tim.on(TencentCloudChat.EVENT.SDK_NOT_READY, () => {
    console.log('TIM Event: SDK_NOT_READY');
    isSDKReady = false;
});

tim.on(TencentCloudChat.EVENT.KICKED_OUT, () => {
    console.log('TIM Event: KICKED_OUT');
    isSDKReady = false;
});

export const loginTIM = (userID: string, userSig: string) => {
    return new Promise((resolve) => {
        if (isSDKReady) {
            console.log('TIM SDK is already ready. Skipping login wait.');
            resolve(true);
            return;
        }

        console.log('TIM Login called, waiting for SDK_READY...');

        // 5s timeout to prevent hanging
        const timeout = setTimeout(() => {
            console.warn('TIM Login Timeout (waiting for SDK_READY). Proceeding anyway (might fail).');
            tim.off(TencentCloudChat.EVENT.SDK_READY, onReady);
            resolve(false);
        }, 5000);

        const onReady = () => {
            clearTimeout(timeout);
            tim.off(TencentCloudChat.EVENT.SDK_READY, onReady);
            console.log('TIM Event: SDK_READY received. Login sequence complete.');
            resolve(true);
        };

        tim.on(TencentCloudChat.EVENT.SDK_READY, onReady);

        tim.login({ userID, userSig }).then(() => {
            console.log('TIM login() API returned success. Still waiting for SDK_READY event.');
        }).catch((error: any) => {
            // Error 6014: Repeat Login.
            if (error?.code === 6014) {
                console.log('TIM login() warning: Repeat Login (6014). Continuing to wait for SDK_READY.');
            } else {
                console.error('TIM login() failed:', error);
            }
        });
    });
};

// Helper function to wait for SDK ready
const waitForSDKReady = (timeoutMs: number = 5000): Promise<boolean> => {
    return new Promise((resolve) => {
        if (isSDKReady) {
            resolve(true);
            return;
        }

        const startTime = Date.now();
        const checkInterval = setInterval(() => {
            if (isSDKReady) {
                clearInterval(checkInterval);
                resolve(true);
            } else if (Date.now() - startTime > timeoutMs) {
                clearInterval(checkInterval);
                console.warn('TIM SDK ready timeout after', timeoutMs, 'ms');
                resolve(false);
            }
        }, 100);
    });
};

// Cache for TIM groups - used when SDK isn't ready yet but groups were fetched earlier
let cachedTIMGroups: any[] = [];

export const getTIMGroups = async () => {
    // If SDK is ready, fetch fresh data
    if (isSDKReady) {
        try {
            const res = await tim.getGroupList();
            console.log('TIM getGroupList success. Count:', res.data.groupList.length);
            // Update cache
            cachedTIMGroups = res.data.groupList;
            return res.data.groupList;
        } catch (error) {
            console.error('TIM getGroupList failed', error);
            // Return cached data if available
            if (cachedTIMGroups.length > 0) {
                console.log('Returning cached TIM groups:', cachedTIMGroups.length);
                return cachedTIMGroups;
            }
            return [];
        }
    }

    // SDK not ready - try to wait
    console.log('getTIMGroups: SDK not ready, waiting...');
    const ready = await waitForSDKReady(5000);

    if (ready) {
        console.log('getTIMGroups: SDK is now ready');
        try {
            const res = await tim.getGroupList();
            console.log('TIM getGroupList success. Count:', res.data.groupList.length);
            cachedTIMGroups = res.data.groupList;
            return res.data.groupList;
        } catch (error) {
            console.error('TIM getGroupList failed', error);
            return cachedTIMGroups.length > 0 ? cachedTIMGroups : [];
        }
    }

    // SDK still not ready - return cached data if available
    if (cachedTIMGroups.length > 0) {
        console.log('getTIMGroups: SDK not ready, returning cached groups:', cachedTIMGroups.length);
        return cachedTIMGroups;
    }

    console.warn('getTIMGroups: SDK still not ready and no cache. Returning [].');
    return [];
};

// Export function to update cache from outside (e.g., ClassManagement)
export const setCachedTIMGroups = (groups: any[]) => {
    cachedTIMGroups = groups;
    console.log('TIM groups cache updated:', groups.length);
};

export const sendMessage = async (to: string, text: string, type: 'C2C' | 'GROUP' = 'GROUP') => {
    if (!isSDKReady) {
        console.error('sendMessage failed: SDK not ready');
        return null;
    }
    try {
        const message = tim.createTextMessage({
            to: to,
            conversationType: type === 'GROUP' ? TencentCloudChat.TYPES.CONV_GROUP : TencentCloudChat.TYPES.CONV_C2C,
            payload: {
                text: text
            }
        });
        const res = await tim.sendMessage(message);
        console.log('TIM sendMessage success:', res);
        return res.data.message; // Return the message object for UI update
    } catch (error) {
        console.error('TIM sendMessage failed:', error);
        throw error;
    }
};

export const getMessageList = async (groupID: string) => {
    if (!isSDKReady) return [];
    try {
        const res = await tim.getMessageList({ conversationID: `GROUP${groupID}` });
        console.log('TIM getMessageList success:', res.data.messageList.length);
        return res.data.messageList;
    } catch (error) {
        console.error('TIM getMessageList failed:', error);
        return [];
    }
};

export const getGroupMemberList = async (groupID: string) => {
    if (!isSDKReady) return [];
    try {
        const res = await tim.getGroupMemberList({ groupID, count: 30, offset: 0 });
        console.log('TIM getGroupMemberList success:', res.data.memberList.length);
        return res.data.memberList;
    } catch (error) {
        console.error('TIM getGroupMemberList failed:', error);
        return [];
    }
};

export const sendImageMessage = async (to: string, file: HTMLInputElement | File, type: 'C2C' | 'GROUP' = 'GROUP') => {
    if (!isSDKReady) {
        console.error('sendImageMessage failed: SDK not ready');
        return null;
    }
    try {
        const message = tim.createImageMessage({
            to: to,
            conversationType: type === 'GROUP' ? TencentCloudChat.TYPES.CONV_GROUP : TencentCloudChat.TYPES.CONV_C2C,
            payload: {
                file: file
            },
            onProgress: (percent: number) => {
                console.log('Image upload progress:', percent);
            }
        });
        const res = await tim.sendMessage(message);
        console.log('TIM sendImageMessage success:', res);
        return res.data.message;
    } catch (error) {
        console.error('TIM sendImageMessage failed:', error);
        throw error;
    }
};

export const sendFileMessage = async (to: string, file: HTMLInputElement | File, type: 'C2C' | 'GROUP' = 'GROUP') => {
    if (!isSDKReady) {
        console.error('sendFileMessage failed: SDK not ready');
        return null;
    }
    try {
        const message = tim.createFileMessage({
            to: to,
            conversationType: type === 'GROUP' ? TencentCloudChat.TYPES.CONV_GROUP : TencentCloudChat.TYPES.CONV_C2C,
            payload: {
                file: file
            },
            onProgress: (percent: number) => {
                console.log('File upload progress:', percent);
            }
        });
        const res = await tim.sendMessage(message);
        console.log('TIM sendFileMessage success:', res);
        return res.data.message;
    } catch (error) {
        console.error('TIM sendFileMessage failed:', error);
        throw error;
    }
};

export const dismissGroup = async (groupID: string) => {
    if (!isSDKReady) throw new Error('SDK not ready');
    try {
        const res = await tim.dismissGroup(groupID);
        console.log('TIM dismissGroup success:', res);
        return res;
    } catch (error) {
        console.error('TIM dismissGroup failed:', error);
        throw error;
    }
};

export const quitGroup = async (groupID: string) => {
    if (!isSDKReady) throw new Error('SDK not ready');
    try {
        const res = await tim.quitGroup(groupID);
        console.log('TIM quitGroup success:', res);
        return res;
    } catch (error) {
        console.error('TIM quitGroup failed:', error);
        throw error;
    }
};

export const changeGroupOwner = async (groupID: string, newOwnerID: string) => {
    if (!isSDKReady) throw new Error('SDK not ready');
    try {
        const res = await tim.changeGroupOwner({
            groupID: groupID,
            newOwnerID: newOwnerID
        });
        console.log('TIM changeGroupOwner success:', res);
        return res;
    } catch (error) {
        console.error('TIM changeGroupOwner failed:', error);
        throw error;
    }
};
