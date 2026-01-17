import { useState, useEffect } from 'react';
import { X, Users, Check } from 'lucide-react';
import { invoke } from '@tauri-apps/api/core';
import { createGroup, invalidateTIMGroupsCache } from '../../utils/tim';

interface CreateNormalGroupModalProps {
    isOpen: boolean;
    onClose: () => void;
    userInfo?: any;
    onSuccess?: () => void;
    // friends prop removed, we fetch internally
}

interface FriendInfo {
    id: string;
    name: string;
    avatar: string;
}

const CreateNormalGroupModal = ({ isOpen, onClose, userInfo, onSuccess }: CreateNormalGroupModalProps) => {
    const [groupName, setGroupName] = useState('');
    const [isLoading, setIsLoading] = useState(false);
    const [selectedFriends, setSelectedFriends] = useState<string[]>([]);
    const [friendList, setFriendList] = useState<FriendInfo[]>([]);
    const [loadingFriends, setLoadingFriends] = useState(false);

    // Fetch friends when modal opens
    useEffect(() => {
        if (isOpen && userInfo?.id_number) {
            fetchFriends();
            setGroupName('');
            setSelectedFriends([]);
            setIsLoading(false);
        }
    }, [isOpen, userInfo]);

    const fetchFriends = async () => {
        setLoadingFriends(true);
        try {
            const token = localStorage.getItem('token');
            const resText = await invoke<string>('get_user_friends', {
                idCard: userInfo.id_number,
                token
            });
            const res = JSON.parse(resText);
            if (res.friends) {
                const mapped = res.friends.map((f: any) => ({
                    id: f.teacher_info?.teacher_unique_id || f.teacher_unique_id,
                    name: f.user_details?.name || f.teacher_info?.name || f.name || '未知好友',
                    avatar: f.user_details?.avatar || f.avatar || ''
                }));
                setFriendList(mapped);
            }
        } catch (e) {
            console.error('Failed to fetch friends:', e);
        } finally {
            setLoadingFriends(false);
        }
    };

    if (!isOpen) return null;

    const handleCreate = async () => {
        if (!groupName.trim()) {
            return;
        }

        setIsLoading(true);
        try {
            console.log('[CreateNormalGroup] Creating group via SDK:', groupName.trim());

            // Use SDK to create group with initial members
            const newGroup = await createGroup({
                name: groupName.trim(),
                type: 'Public',
                memberList: selectedFriends.map(id => ({ userID: id }))
            });

            console.log('[CreateNormalGroup] Group Created Successfully via SDK. GroupId:', newGroup.groupID);

            onSuccess?.();

            // Just one refresh is enough since SDK already knows about the group
            const { emit } = await import('@tauri-apps/api/event');
            invalidateTIMGroupsCache();
            await emit('refresh-class-list');

            onClose();
        } catch (error) {
            console.error("[CreateNormalGroup] Failed to create group:", error);
            alert(`创建失败: ${error}`);
        } finally {
            setIsLoading(false);
        }
    };

    return (
        <div className="fixed inset-0 z-[60] flex items-center justify-center bg-black/30 backdrop-blur-sm animate-in fade-in duration-300 font-sans">
            <div className="bg-paper/95 backdrop-blur-xl rounded-[2rem] shadow-2xl w-[500px] flex flex-col overflow-hidden animate-in zoom-in-95 duration-200 border border-white/50 ring-1 ring-sage-100/50">

                {/* Header */}
                <div className="px-8 py-6 border-b border-sage-100/50 bg-white/30 backdrop-blur-md flex items-center justify-between">
                    <div className="flex items-center gap-3">
                        <div className="w-10 h-10 rounded-2xl bg-gradient-to-br from-indigo-400 to-indigo-600 flex items-center justify-center shadow-lg shadow-indigo-500/20">
                            <Users className="w-5 h-5 text-white" />
                        </div>
                        <div>
                            <span className="font-bold text-xl text-ink-800 tracking-tight">创建普通群</span>
                            <p className="text-sm font-medium text-ink-400 mt-0.5">邀请好友加入群聊</p>
                        </div>
                    </div>
                    <button
                        onClick={onClose}
                        className="w-10 h-10 flex items-center justify-center text-sage-400 hover:text-clay-600 hover:bg-clay-50 rounded-full transition-all duration-300"
                    >
                        <X size={24} />
                    </button>
                </div>

                {/* Content */}
                <div className="p-8 flex flex-col gap-6 bg-white/40">
                    <div className="flex flex-col gap-2 group">
                        <label className="text-xs font-bold text-ink-400 ml-1 uppercase tracking-wider group-focus-within:text-sage-600 transition-colors">群组名称</label>
                        <input
                            type="text"
                            value={groupName}
                            onChange={(e) => setGroupName(e.target.value)}
                            placeholder="请输入群组名称..."
                            className="w-full px-5 py-4 rounded-2xl border border-sage-200 bg-white text-ink-800 outline-none focus:ring-4 focus:ring-sage-100 focus:border-sage-400 transition-all placeholder:text-sage-300 font-medium shadow-sm"
                            maxLength={30}
                            autoFocus
                        />
                        <p className="text-xs font-medium text-ink-300 text-right pr-2">{groupName.length}/30</p>
                    </div>

                    <div className="flex flex-col gap-2 flex-1 min-h-0">
                        <label className="text-xs font-bold text-ink-400 ml-1 uppercase tracking-wider">邀请初始成员 <span className="text-sage-500 bg-sage-50 px-1.5 py-0.5 rounded-md ml-1">{selectedFriends.length}</span></label>
                        <div className="border border-white/50 ring-1 ring-sage-50 rounded-2xl bg-white/60 overflow-y-auto h-[240px] p-3 custom-scrollbar shadow-inner backdrop-blur-sm">
                            {loadingFriends ? (
                                <div className="h-full flex flex-col items-center justify-center text-sage-400 text-sm gap-2 font-bold animate-pulse">
                                    <div className="w-6 h-6 border-2 border-sage-200 border-t-sage-500 rounded-full animate-spin" />
                                    加载好友列表...
                                </div>
                            ) : friendList.length === 0 ? (
                                <div className="h-full flex flex-col items-center justify-center text-sage-300 gap-3">
                                    <Users size={32} className="opacity-50" />
                                    <span className="text-sm font-bold">暂无好友可邀请</span>
                                </div>
                            ) : (
                                <div className="space-y-1.5">
                                    {friendList.map(friend => {
                                        const isSelected = selectedFriends.includes(friend.id);
                                        return (
                                            <div
                                                key={friend.id}
                                                onClick={() => {
                                                    setSelectedFriends(prev =>
                                                        prev.includes(friend.id)
                                                            ? prev.filter(id => id !== friend.id)
                                                            : [...prev, friend.id]
                                                    );
                                                }}
                                                className={`flex items-center gap-4 p-3 rounded-xl cursor-pointer transition-all border group ${isSelected
                                                    ? 'bg-indigo-50 border-indigo-200 shadow-sm'
                                                    : 'bg-transparent hover:bg-white/80 border-transparent hover:border-sage-50 hover:shadow-sm'
                                                    }`}
                                            >
                                                <div className="relative">
                                                    {friend.avatar ? (
                                                        <img src={friend.avatar} alt="" className="w-10 h-10 rounded-full object-cover border border-white shadow-sm" />
                                                    ) : (
                                                        <div className="w-10 h-10 rounded-full bg-indigo-100 flex items-center justify-center text-indigo-500 text-sm font-bold border border-white shadow-sm">
                                                            {friend.name.charAt(0)}
                                                        </div>
                                                    )}
                                                    {isSelected && (
                                                        <div className="absolute -bottom-1 -right-1 w-5 h-5 bg-indigo-500 rounded-full flex items-center justify-center border-2 border-white shadow-sm animate-in zoom-in-75">
                                                            <Check size={10} className="text-white" />
                                                        </div>
                                                    )}
                                                </div>
                                                <span className={`flex-1 text-sm font-bold transition-colors ${isSelected ? 'text-indigo-700' : 'text-ink-700'}`}>{friend.name}</span>
                                            </div>
                                        );
                                    })}
                                </div>
                            )}
                        </div>
                    </div>
                </div>


                {/* Footer */}
                <div className="p-6 bg-white/50 backdrop-blur-md border-t border-sage-100/50 flex justify-end gap-3">
                    <button
                        onClick={onClose}
                        className="px-6 py-3 text-ink-500 bg-white border border-sage-200 rounded-2xl hover:bg-sage-50 font-bold text-sm transition-all shadow-sm hover:shadow active:scale-95"
                    >
                        取消
                    </button>
                    <button
                        onClick={handleCreate}
                        disabled={isLoading || !groupName.trim()}
                        className="px-8 py-3 bg-gradient-to-r from-indigo-500 to-indigo-600 text-white rounded-2xl hover:from-indigo-600 hover:to-indigo-700 font-bold shadow-lg shadow-indigo-500/20 disabled:opacity-50 disabled:cursor-not-allowed disabled:shadow-none flex items-center gap-2 transition-all active:scale-95"
                    >
                        {isLoading ? (
                            <>
                                <div className="w-4 h-4 border-2 border-white/30 border-t-white rounded-full animate-spin" />
                                <span>创建中...</span>
                            </>
                        ) : (
                            <span>确认创建</span>
                        )}
                    </button>
                </div>

            </div>
        </div >
    );
};

export default CreateNormalGroupModal;
