import TencentCloudChat from '@tencentcloud/chat';

const SDKAppID = 1600111046;

export const tim = TencentCloudChat.create({
    SDKAppID
});

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

export const getTIMGroups = async () => {
    if (!isSDKReady) {
        console.warn('getTIMGroups called but SDK is NOT READY. Returning [].');
        return [];
    }
    try {
        const res = await tim.getGroupList();
        console.log('TIM getGroupList success. Count:', res.data.groupList.length);
        return res.data.groupList;
    } catch (error) {
        console.error('TIM getGroupList failed', error);
        return [];
    }
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
