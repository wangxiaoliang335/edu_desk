import React, { useState, useEffect } from 'react';
import { X, School, BookOpen, Users2, Activity, Heart, ArrowRight, Building, Save, Trash2, Plus, Upload, Download, CheckCircle2 } from 'lucide-react';
import { invoke } from '@tauri-apps/api/core';

interface UserInfo {
    name?: string;
    school_name?: string;
    address?: string;
    avatar?: string;
    [key: string]: any;
}

interface SchoolInfoModalProps {
    isOpen: boolean;
    onClose: () => void;
    userInfo: UserInfo | null;
}

// Reusable Components
const TabButton = ({ active, icon: Icon, label, onClick }: { active: boolean; icon: any; label: string; onClick: () => void }) => (
    <button
        onClick={onClick}
        className={`w-full flex items-center gap-3 px-4 py-3 rounded-xl transition-all duration-300 group relative overflow-hidden ${active
            ? 'bg-white text-sage-600 font-bold shadow-md shadow-sage-200/50 ring-1 ring-sage-100'
            : 'text-ink-400 hover:bg-sage-50/50 hover:text-ink-600'
            }`}
    >
        <Icon size={20} className={`transition-transform duration-300 ${active ? 'scale-110 text-sage-500' : 'group-hover:scale-110'}`} />
        <span className="font-bold tracking-wide">{label}</span>
        {active && <div className="absolute left-0 top-1/2 -translate-y-1/2 w-1 h-6 bg-sage-400 rounded-r-full" />}
    </button>
);

const InputField = ({ label, value, onChange, placeholder, readOnly, actionButton }: any) => (
    <div className="group">
        <label className="block text-xs font-bold text-ink-400 mb-1.5 ml-1 transition-colors group-focus-within:text-sage-600 uppercase tracking-wider">{label}</label>
        <div className="relative flex gap-2">
            <input
                type="text"
                value={value}
                onChange={onChange}
                readOnly={readOnly}
                className={`w-full bg-white border border-sage-200 rounded-xl px-4 py-3 text-ink-800 placeholder-sage-300 focus:outline-none focus:border-sage-400 focus:ring-4 focus:ring-sage-100 transition-all duration-300 shadow-sm ${readOnly ? 'bg-sage-50/50 cursor-not-allowed text-ink-400' : ''}`}
                placeholder={placeholder}
            />
            {actionButton}
        </div>
    </div>
);

// --- Sub-components for tabs ---

const SchoolInfoTab: React.FC<{
    userInfo: UserInfo | null;
    orgCode: string;
    setOrgCode: (code: string) => void;
}> = ({ userInfo, orgCode, setOrgCode }) => {
    const [schoolName, setSchoolName] = useState(userInfo?.school_name || '');
    const [address, setAddress] = useState(userInfo?.address || '');
    const [loading, setLoading] = useState(false);

    useEffect(() => {
        if (userInfo?.school_name) {
            setSchoolName(userInfo.school_name);
            fetchSchoolInfo(userInfo.school_name);
        }
        if (userInfo?.address) {
            setAddress(userInfo.address);
        }
    }, [userInfo]);

    const fetchSchoolInfo = async (name: string) => {
        try {
            const response = await invoke<string>('get_school_by_name', { name });
            const parsed = JSON.parse(response);
            const data = parsed.data || parsed;

            if (data.code === 200 && data.schools?.length > 0) {
                const school = data.schools[0];
                setOrgCode(school.id);
                if (school.address) {
                    setAddress(school.address);
                }
            }
        } catch (error) {
            console.error('Failed to fetch school info:', error);
        }
    };

    const handleGetCode = async () => {
        if (!schoolName) {
            alert("学校名不能为空");
            return;
        }
        setLoading(true);
        try {
            const response = await invoke<string>('get_unique_6_digit');
            const data = JSON.parse(response);
            if (data.data?.code) {
                const newCode = data.data.code;
                setOrgCode(newCode);

                const updateResp = await invoke<string>('update_school_info', {
                    id: newCode,
                    name: schoolName,
                    address: address
                });
                const updateData = JSON.parse(updateResp);
                if (updateData.code === 200) {
                    alert(`组织代码获取成功：${newCode}，并已保存学校信息。`);
                } else {
                    alert(`获取代码成功，但保存信息失败：${updateData.message}`);
                }
            } else {
                alert("获取组织代码失败");
            }

        } catch (error) {
            console.error(error);
            alert("操作失败");
        } finally {
            setLoading(false);
        }
    };

    return (
        <div className="p-8 max-w-2xl mx-auto space-y-8 animate-in fade-in slide-in-from-bottom-4 duration-500">
            <div className="flex items-center gap-4 mb-8 p-6 bg-gradient-to-br from-sage-50 to-white rounded-3xl border border-sage-100 shadow-sm">
                <div className="w-16 h-16 rounded-2xl bg-sage-500 text-white flex items-center justify-center shadow-lg shadow-sage-500/30">
                    <School size={32} />
                </div>
                <div>
                    <h3 className="text-xl font-bold text-ink-800">基础信息配置</h3>
                    <p className="text-sm text-ink-400 mt-1 font-medium">完善学校基本资料，获取用于系统识别的唯一组织代码。</p>
                </div>
            </div>

            <div className="space-y-6 bg-white/60 p-8 rounded-3xl shadow-sm border border-white/50 ring-1 ring-sage-50 backdrop-blur-sm">
                <InputField
                    label="学校名称"
                    value={schoolName}
                    onChange={(e: any) => setSchoolName(e.target.value)}
                    placeholder="请输入学校完整名称"
                />
                <InputField
                    label="学校地址"
                    value={address}
                    onChange={(e: any) => setAddress(e.target.value)}
                    placeholder="请输入详细地址"
                />
                <InputField
                    label="组织代码 (System ID)"
                    value={orgCode}
                    readOnly
                    placeholder="系统自动生成"
                    actionButton={
                        <button
                            onClick={handleGetCode}
                            disabled={loading || !!orgCode}
                            className="bg-sage-600 hover:bg-sage-700 text-white px-6 rounded-xl font-bold transition-all disabled:opacity-50 disabled:cursor-not-allowed whitespace-nowrap shadow-md hover:shadow-lg disabled:shadow-none active:scale-95 flex items-center gap-2"
                        >
                            {loading ? <Activity className="animate-spin" size={18} /> : <CheckCircle2 size={18} />}
                            {loading ? '获取中...' : '获取代码'}
                        </button>
                    }
                />
            </div>
        </div>
    );
};

interface ClassItem {
    id?: string;
    school_stage: string;
    grade: string;
    class_name: string;
    class_code: string;
    checked?: boolean;
}

const ClassManagementTab: React.FC<{ schoolId: string }> = ({ schoolId }) => {
    const [classes, setClasses] = useState<ClassItem[]>([]);
    const [loading, setLoading] = useState(false);

    useEffect(() => {
        if (schoolId) {
            fetchClasses();
        }
    }, [schoolId]);

    const fetchClasses = async () => {
        if (!schoolId) return;
        setLoading(true);
        console.log(`Fetching classes for schoolId: ${schoolId}`);
        try {
            const response = await invoke<string>('get_classes_by_prefix', { prefix: schoolId });
            console.log("Fetch Classes Response:", response);
            const parsed = JSON.parse(response);
            const data = parsed.data || parsed;

            if (data.code === 200 && data.classes) {
                console.log(`Loaded ${data.classes.length} classes`);
                setClasses(data.classes.map((c: any) => ({
                    ...c,
                    checked: false
                })));
            } else {
                console.log("No classes found or error code:", data.code);
            }
        } catch (e) {
            console.error("Failed to fetch classes:", e);
        } finally {
            setLoading(false);
        }
    };

    const handleAddRow = () => {
        setClasses([...classes, { school_stage: '小学', grade: '一年级', class_name: '一班', class_code: '', checked: false }]);
    };

    const handleDelete = async () => {
        const toDelete = classes.filter(c => c.checked && c.class_code);
        if (toDelete.length === 0) {
            const newClasses = classes.filter(c => !c.checked);
            setClasses(newClasses);
            return;
        }

        if (confirm(`确定要删除选中的 ${toDelete.length} 个已保存班级吗？`)) {
            try {
                const payload = toDelete.map(c => ({
                    class_code: c.class_code,
                    schoolid: schoolId,
                    school_stage: c.school_stage,
                    grade: c.grade,
                    class_name: c.class_name
                }));

                const response = await invoke<string>('delete_classes', { classes: payload });
                const data = JSON.parse(response);
                if (data.data?.code === 200) {
                    alert(`删除成功，共删除 ${data.data.deleted_count} 条`);
                    fetchClasses();
                } else {
                    alert(`删除失败：${data.message}`);
                }
            } catch (e) {
                console.error(e);
                alert("删除出错");
            }
        }
    };

    const handleGenerate = async () => {
        if (!schoolId) {
            alert("请先获取组织代码！");
            return;
        }

        const newRows = classes.filter(c => !c.class_code);
        if (newRows.length === 0) {
            alert("没有需要生成编号的新班级");
            return;
        }

        // Check for duplicates
        const existingSignatures = new Set(
            classes
                .filter(c => c.class_code)
                .map(c => `${c.school_stage}-${c.grade}-${c.class_name}`)
        );

        const newSignatures = new Set<string>();

        for (const row of newRows) {
            const sig = `${row.school_stage}-${row.grade}-${row.class_name}`;

            if (existingSignatures.has(sig)) {
                alert(`班级 "${row.school_stage} ${row.grade} ${row.class_name}" 已存在，无法重复添加！`);
                return;
            }

            if (newSignatures.has(sig)) {
                alert(`新增列表中包含重复的班级："${row.school_stage} ${row.grade} ${row.class_name}"，请检查！`);
                return;
            }
            newSignatures.add(sig);
        }

        try {
            const payload = newRows.map(c => ({
                school_stage: c.school_stage,
                grade: c.grade,
                class_name: c.class_name,
                schoolid: schoolId
            }));

            const response = await invoke<string>('update_classes', { classes: payload });
            const data = JSON.parse(response);
            if (data.data?.code === 200) {
                alert("生成并保存成功！");
                fetchClasses();
            } else {
                alert(`保存失败：${data.message || data.data?.message}`);
            }

        } catch (e) {
            console.error(e);
            alert("保存出错");
        }
    };

    const updateField = (index: number, field: keyof ClassItem, value: any) => {
        const newClasses = [...classes];
        newClasses[index] = { ...newClasses[index], [field]: value };
        setClasses(newClasses);
    };

    return (
        <div className="flex flex-col h-full">
            {/* Toolbar */}
            <div className="h-16 flex items-center px-6 gap-3 border-b border-sage-100 bg-white/50 backdrop-blur-sm">
                <button onClick={handleAddRow} className="flex items-center gap-2 px-4 py-2 bg-sage-50 hover:bg-white hover:text-sage-600 rounded-xl text-ink-500 text-sm transition-all border border-sage-200 hover:border-sage-300 shadow-sm font-bold">
                    <Plus size={16} /> 添加班级
                </button>
                <button onClick={handleDelete} className="flex items-center gap-2 px-4 py-2 bg-clay-50 hover:bg-clay-100 hover:text-clay-600 rounded-xl text-ink-500 text-sm transition-all border border-clay-100 hover:border-clay-200 shadow-sm font-bold">
                    <Trash2 size={16} /> 删除选中
                </button>
                <div className="flex-1" />
                <button onClick={handleGenerate} className="flex items-center gap-2 px-5 py-2 bg-sage-500 hover:bg-sage-600 rounded-xl text-white text-sm font-bold transition-all shadow-md shadow-sage-500/20 active:scale-95 hover:shadow-lg">
                    <Save size={16} />
                    <span>生成编号并保存</span>
                </button>
            </div>

            {/* Table Area */}
            <div className="flex-1 overflow-hidden flex flex-col p-6">
                <div className="flex-1 bg-white/70 rounded-3xl border border-white/50 ring-1 ring-sage-100 overflow-hidden flex flex-col shadow-sm backdrop-blur-md">
                    {/* Header */}
                    <div className="flex items-center text-xs font-bold text-ink-400 bg-sage-50/50 h-12 border-b border-sage-100 uppercase tracking-wider">
                        <div className="w-16 flex justify-center">
                            <input type="checkbox" className="rounded border-sage-300 text-sage-600 focus:ring-sage-500 focus:ring-offset-0 cursor-pointer" onChange={(e) => setClasses(classes.map(c => ({ ...c, checked: e.target.checked })))} />
                        </div>
                        <div className="flex-1 px-4 border-l border-sage-100/50">学段</div>
                        <div className="flex-1 px-4 border-l border-sage-100/50">年级</div>
                        <div className="flex-1 px-4 border-l border-sage-100/50">班级名称</div>
                        <div className="flex-1 px-4 border-l border-sage-100/50">班级编号</div>
                    </div>

                    {/* Body */}
                    <div className="flex-1 overflow-y-auto custom-scrollbar p-0">
                        {loading ? (
                            <div className="flex items-center justify-center h-full text-sage-400 gap-2 font-bold">
                                <Activity className="animate-spin" /> 加载中...
                            </div>
                        ) : classes.length === 0 ? (
                            <div className="flex flex-col items-center justify-center h-full text-sage-300 gap-3">
                                <BookOpen size={48} className="opacity-20" />
                                <p className="font-bold">暂无班级数据</p>
                            </div>
                        ) : (
                            classes.map((cls, idx) => (
                                <div key={idx} className={`group flex items-center text-sm transition-all duration-150 border-b border-sage-50 last:border-0 ${cls.checked ? 'bg-sage-50/50' : 'hover:bg-white/80'}`}>
                                    <div className="w-16 flex justify-center py-3">
                                        <input
                                            type="checkbox"
                                            checked={!!cls.checked}
                                            onChange={(e) => updateField(idx, 'checked', e.target.checked)}
                                            className="rounded border-sage-300 text-sage-600 focus:ring-sage-500 focus:ring-offset-0 cursor-pointer"
                                        />
                                    </div>
                                    <div className="flex-1 px-2">
                                        <input
                                            className="w-full bg-transparent text-ink-800 focus:bg-white rounded px-2 py-1.5 border-none focus:ring-1 focus:ring-sage-400 transition-all font-bold"
                                            value={cls.school_stage}
                                            onChange={e => updateField(idx, 'school_stage', e.target.value)}
                                        />
                                    </div>
                                    <div className="flex-1 px-2">
                                        <input
                                            className="w-full bg-transparent text-ink-800 focus:bg-white rounded px-2 py-1.5 border-none focus:ring-1 focus:ring-sage-400 transition-all font-medium"
                                            value={cls.grade}
                                            onChange={e => updateField(idx, 'grade', e.target.value)}
                                        />
                                    </div>
                                    <div className="flex-1 px-2">
                                        <input
                                            className="w-full bg-transparent text-ink-800 focus:bg-white rounded px-2 py-1.5 border-none focus:ring-1 focus:ring-sage-400 transition-all font-medium"
                                            value={cls.class_name}
                                            onChange={e => updateField(idx, 'class_name', e.target.value)}
                                        />
                                    </div>
                                    <div className="flex-1 px-4 py-3 text-ink-400 font-mono text-xs font-medium">
                                        {cls.class_code || '---'}
                                    </div>
                                </div>
                            ))
                        )}
                    </div>
                </div>
            </div>
        </div>
    );
};

interface MemberItem {
    id?: string;
    is_administrator?: string; // "1" YES, "0" NO
    teacher_unique_id?: string;
    name: string;
    phone: string;
    id_number?: string;
    school_name?: string;
    checked?: boolean;
}

const MemberManagementTab: React.FC<{ schoolId: string }> = ({ schoolId }) => {
    const [members, setMembers] = useState<MemberItem[]>([]);
    const [loading, setLoading] = useState(false);

    useEffect(() => {
        if (schoolId) {
            fetchMembers();
        }
    }, [schoolId]);

    const fetchMembers = async () => {
        if (!schoolId) return;
        setLoading(true);
        console.log(`Fetching members for schoolId: ${schoolId}`);
        try {
            const response = await invoke<string>('get_list_teachers', { schoolId });
            console.log("Fetch Members Response:", response);
            const parsed = JSON.parse(response);
            const data = parsed.data || parsed;

            if (data.code === 200 && data.teachers) {
                console.log(`Loaded ${data.teachers.length} members`);
                setMembers(data.teachers.map((m: any) => ({
                    ...m,
                    name: m.name || '',
                    phone: m.phone || '',
                    checked: false
                })));
            } else {
                console.log("No members found or error code:", data.code);
            }
        } catch (e) {
            console.error("Failed to fetch members:", e);
        } finally {
            setLoading(false);
        }
    };

    const handleAddRow = () => {
        setMembers([...members, {
            name: '新成员',
            phone: '',
            is_administrator: '0',
            checked: false,
            teacher_unique_id: ''
        }]);
    };

    const handleDelete = async () => {
        const toDelete = members.filter(m => m.checked && m.phone);
        if (toDelete.length === 0) {
            const newMembers = members.filter(m => !m.checked);
            setMembers(newMembers);
            return;
        }

        if (confirm(`确定要删除选中的 ${toDelete.length} 个成员吗？`)) {
            try {
                const phones = toDelete.map(m => m.phone);
                const response = await invoke<string>('delete_teacher', { phones });
                const data = JSON.parse(response);
                if (data.code === 200) {
                    alert("删除成功");
                    fetchMembers();
                } else {
                    alert(`删除失败：${data.message}`);
                }
            } catch (e) {
                console.error(e);
                alert("删除出错");
            }
        }
    };

    const handleSave = async () => {
        const newMembers = members.filter(m => !m.teacher_unique_id && m.phone && m.name);
        if (newMembers.length === 0) {
            alert("没有填写完整的新成员数据需要保存");
            return;
        }

        try {
            const payload = newMembers.map(m => ({
                phone: m.phone,
                name: m.name,
                school_id: schoolId,
                password: "123",
                is_administrator: m.is_administrator || "0"
            }));

            const response = await invoke<string>('add_teacher', { teachers: payload });
            const data = JSON.parse(response);
            if (data.code === 200) {
                alert("保存成功");
                fetchMembers();
            } else {
                alert(`保存失败：${data.message}`);
            }

        } catch (e) {
            console.error(e);
            alert("保存出错");
        }
    };

    const updateField = (index: number, field: keyof MemberItem, value: any) => {
        const newMembers = [...members];
        newMembers[index] = { ...newMembers[index], [field]: value };
        setMembers(newMembers);
    };

    return (
        <div className="flex flex-col h-full">
            {/* Toolbar */}
            <div className="h-16 flex items-center px-6 gap-3 border-b border-sage-100 bg-white/50 backdrop-blur-sm">
                <button onClick={handleAddRow} className="flex items-center gap-2 px-4 py-2 bg-sage-50 hover:bg-white hover:text-sage-600 rounded-xl text-ink-500 text-sm transition-all border border-sage-200 hover:border-sage-300 shadow-sm font-bold">
                    <Plus size={16} /> 添加成员
                </button>
                <div className="h-6 w-[1px] bg-sage-200/50 mx-1" />
                <button className="flex items-center gap-2 px-4 py-2 bg-white hover:bg-sage-50 rounded-xl text-ink-500 text-sm transition-all border border-sage-200 hover:text-ink-700 font-bold">
                    <Upload size={16} /> 导入
                </button>
                <button className="flex items-center gap-2 px-4 py-2 bg-white hover:bg-sage-50 rounded-xl text-ink-500 text-sm transition-all border border-sage-200 hover:text-ink-700 font-bold">
                    <Download size={16} /> 导出
                </button>
                <div className="h-6 w-[1px] bg-sage-200/50 mx-1" />
                <button onClick={handleDelete} className="flex items-center gap-2 px-4 py-2 bg-clay-50 hover:bg-clay-100 hover:text-clay-600 rounded-xl text-ink-500 text-sm transition-all border border-clay-100 hover:border-clay-200 shadow-sm font-bold">
                    <Trash2 size={16} /> 删除
                </button>
                <div className="flex-1" />
                <button onClick={handleSave} className="flex items-center gap-2 px-5 py-2 bg-sage-500 hover:bg-sage-600 rounded-xl text-white text-sm font-bold transition-all shadow-md shadow-sage-500/20 active:scale-95 hover:shadow-lg">
                    <Save size={16} />
                    <span>保存并生成</span>
                </button>
            </div>

            {/* Table Area */}
            <div className="flex-1 overflow-hidden flex flex-col p-6">
                <div className="flex-1 bg-white/70 rounded-3xl border border-white/50 ring-1 ring-sage-100 overflow-hidden flex flex-col shadow-sm backdrop-blur-md">
                    {/* Header */}
                    <div className="flex items-center text-xs font-bold text-ink-400 bg-sage-50/50 h-12 border-b border-sage-100 uppercase tracking-wider">
                        <div className="w-16 p-3 text-center">
                            <input type="checkbox" className="rounded border-sage-300 text-sage-600 focus:ring-sage-500 focus:ring-offset-0 cursor-pointer" onChange={(e) => setMembers(members.map(m => ({ ...m, checked: e.target.checked })))} />
                        </div>
                        <div className="flex-1 px-4 border-l border-sage-100/50">姓名</div>
                        <div className="flex-1 px-4 border-l border-sage-100/50">手机号 (必填)</div>
                        <div className="w-32 text-center border-l border-sage-100/50">管理员</div>
                        <div className="flex-1 px-4 text-center border-l border-sage-100/50">系统编号</div>
                    </div>

                    {/* Body */}
                    <div className="flex-1 overflow-y-auto custom-scrollbar p-0">
                        {loading ? (
                            <div className="flex items-center justify-center h-full text-sage-400 gap-2 font-bold">
                                <Activity className="animate-spin" /> 加载中...
                            </div>
                        ) : members.length === 0 ? (
                            <div className="flex flex-col items-center justify-center h-full text-sage-300 gap-3">
                                <Users2 size={48} className="opacity-20" />
                                <p className="font-bold">暂无成员数据</p>
                            </div>
                        ) : (
                            members.map((m, idx) => (
                                <div key={idx} className={`group flex items-center text-sm transition-all duration-150 border-b border-sage-50 last:border-0 ${m.checked ? 'bg-sage-50/50' : 'hover:bg-white/80'}`}>
                                    <div className="w-16 flex justify-center py-3">
                                        <input
                                            type="checkbox"
                                            checked={!!m.checked}
                                            onChange={(e) => updateField(idx, 'checked', e.target.checked)}
                                            className="rounded border-sage-300 text-sage-600 focus:ring-sage-500 focus:ring-offset-0 cursor-pointer"
                                        />
                                    </div>
                                    <div className="flex-1 px-2">
                                        <input
                                            className="w-full bg-transparent text-ink-800 focus:bg-white rounded px-2 py-1.5 border-none focus:ring-1 focus:ring-sage-400 transition-all font-bold"
                                            value={m.name}
                                            onChange={e => updateField(idx, 'name', e.target.value)}
                                            placeholder="姓名"
                                        />
                                    </div>
                                    <div className="flex-1 px-2">
                                        <input
                                            className="w-full bg-transparent text-ink-800 focus:bg-white rounded px-2 py-1.5 border-none focus:ring-1 focus:ring-sage-400 transition-all font-mono text-ink-600 font-medium tracking-tight"
                                            value={m.phone}
                                            onChange={e => updateField(idx, 'phone', e.target.value)}
                                            placeholder="手机号"
                                        />
                                    </div>
                                    <div className="w-32 flex justify-center">
                                        <div className="relative flex items-center">
                                            <input
                                                type="checkbox"
                                                checked={m.is_administrator === "1"}
                                                onChange={e => updateField(idx, 'is_administrator', e.target.checked ? "1" : "0")}
                                                className="peer sr-only"
                                                id={`admin-cb-${idx}`}
                                            />
                                            <label
                                                htmlFor={`admin-cb-${idx}`}
                                                className="cursor-pointer w-10 h-6 bg-sage-200 peer-checked:bg-sage-500 rounded-full transition-all peer-checked:after:translate-x-full after:content-[''] after:absolute after:top-[2px] after:left-[2px] after:bg-white after:border-sage-200 after:border after:rounded-full after:h-5 after:w-5 after:transition-all shadow-sm"
                                            />
                                        </div>
                                    </div>
                                    <div className="flex-1 px-4 py-3 text-ink-400 font-mono text-xs text-center font-medium">
                                        {m.teacher_unique_id ? (
                                            <span className="bg-sage-50 px-2 py-1 rounded text-sage-600 border border-sage-100">{m.teacher_unique_id}</span>
                                        ) : (
                                            <span className="text-sage-300">待生成</span>
                                        )}
                                    </div>
                                </div>
                            ))
                        )}
                    </div>
                </div>
            </div>
        </div>
    );
};


// Placeholder for other tabs
const PlaceholderTab: React.FC<{ title: string; icon: any }> = ({ title, icon: Icon }) => (
    <div className="flex flex-col items-center justify-center h-full text-gray-400 space-y-4">
        <div className="w-20 h-20 rounded-full bg-gray-50 flex items-center justify-center">
            <Icon size={40} className="opacity-30" />
        </div>
        <p className="text-lg font-medium">{title} 功能开发中...</p>
    </div>
);


const SchoolInfoModal: React.FC<SchoolInfoModalProps> = ({ isOpen, onClose, userInfo }) => {
    const [activeTab, setActiveTab] = useState<'info' | 'class' | 'member' | 'health' | 'growth'>('member');
    const [orgCode, setOrgCode] = useState('');

    useEffect(() => {
        if (isOpen && userInfo?.school_name) {
            fetchSchoolInfo(userInfo.school_name);
        }
    }, [isOpen, userInfo]);

    const fetchSchoolInfo = async (name: string) => {
        try {
            console.log(`[Parent] Fetching school info for: ${name}`);
            const response = await invoke<string>('get_school_by_name', { name });
            const parsed = JSON.parse(response);
            const data = parsed.data || parsed;

            if (data.code === 200 && data.schools?.length > 0) {
                const school = data.schools[0];
                console.log("[Parent] School Found:", school);
                setOrgCode(school.id);
            }
        } catch (error) {
            console.error('Failed to fetch school info:', error);
        }
    };

    if (!isOpen) return null;

    const tabs = [
        { id: 'info', label: '学校信息', icon: School },
        { id: 'class', label: '班级管理', icon: BookOpen },
        { id: 'member', label: '通讯录', icon: Users2 },
        { id: 'health', label: '健康管理', icon: Activity },
        { id: 'growth', label: '成长管理', icon: Heart },
    ] as const;

    return (
        <div className="fixed inset-0 z-50 flex items-center justify-center">
            {/* Backdrop */}
            <div
                className="absolute inset-0 bg-ink-900/20 backdrop-blur-sm animate-in fade-in duration-300"
                onClick={onClose}
            />

            {/* Modal Container */}
            <div className="relative w-[1000px] h-[650px] bg-paper/95 backdrop-blur-xl rounded-[2rem] shadow-2xl flex overflow-hidden border border-white/40 ring-1 ring-sage-100/50 animate-in zoom-in-95 duration-300">

                {/* Close Button - Floats top right */}
                <button
                    onClick={onClose}
                    className="absolute top-5 right-5 text-sage-400 hover:text-clay-600 hover:bg-clay-50 p-2 rounded-full transition-all z-20 group"
                >
                    <X size={20} className="group-hover:rotate-90 transition-transform duration-300" />
                </button>

                {/* Sidebar */}
                <div className="w-[260px] bg-white/50 border-r border-sage-100 flex flex-col pt-10 pb-6 px-4 relative z-10 backdrop-blur-md">
                    <div className="flex-1 space-y-2">
                        {tabs.map((tab) => (
                            <TabButton
                                key={tab.id}
                                active={activeTab === tab.id}
                                icon={tab.icon}
                                label={tab.label}
                                onClick={() => setActiveTab(tab.id as any)}
                            />
                        ))}
                    </div>

                    {/* User Profile Mini - Bottom Sidebar */}
                    <div className="mt-4 pt-4 border-t border-sage-100 flex items-center gap-3 px-2">
                        <div className="w-10 h-10 rounded-full bg-gradient-to-tr from-sage-400 to-sage-600 p-[2px] shadow-md ring-2 ring-white">
                            <img src={userInfo?.avatar || "https://api.dicebear.com/7.x/avataaars/svg?seed=Teacher"} alt="User" className="w-full h-full rounded-full border border-white bg-white object-cover" />
                        </div>
                        <div className="flex flex-col overflow-hidden">
                            <span className="text-sm font-bold text-ink-800 truncate">{userInfo?.name || '教师'}</span>
                            <span className="text-xs text-ink-400 truncate">{userInfo?.school_name || '未绑定学校'}</span>
                        </div>
                    </div>
                </div>

                {/* Main Content Area */}
                <div className="flex-1 flex flex-col min-w-0 bg-transparent relative">

                    {/* Header - Only Title */}
                    <div className="h-20 flex items-center px-8 shrink-0 z-10 border-b border-sage-100/50 bg-white/30 backdrop-blur-sm">
                        <div className="flex flex-col">
                            <h2 className="text-2xl font-bold text-ink-800 tracking-tight">
                                {tabs.find(t => t.id === activeTab)?.label}
                            </h2>
                            <p className="text-xs text-ink-400 font-medium tracking-wide opacity-80">SCHOOL MANAGEMENT</p>
                        </div>
                    </div>

                    {/* Content */}
                    <div className="flex-1 overflow-hidden flex flex-col relative z-0 bg-white/40">
                        {activeTab === 'info' && <SchoolInfoTab userInfo={userInfo} orgCode={orgCode} setOrgCode={setOrgCode} />}
                        {activeTab === 'class' && <ClassManagementTab schoolId={orgCode} />}
                        {activeTab === 'member' && <MemberManagementTab schoolId={orgCode} />}
                        {activeTab === 'health' && <PlaceholderTab title="健康管理" icon={Activity} />}
                        {activeTab === 'growth' && <PlaceholderTab title="成长管理" icon={Heart} />}
                    </div>
                </div>
            </div>
        </div>
    );
};

export default SchoolInfoModal;
