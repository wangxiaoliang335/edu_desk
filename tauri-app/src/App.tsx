import { useState } from "react";
import { HashRouter, Routes, Route } from "react-router-dom";
import Login from "./components/Login";
import Dashboard from "./components/Dashboard";
import ClassChatWindow from "./components/ClassChatWindow";
import ClassScheduleWindow from "./components/ClassScheduleWindow";
import IntercomWindow from "./components/IntercomWindow";
import NormalGroupChatWindow from "./components/NormalGroupChatWindow";
import DesktopFileBox from "./components/DesktopFileBox";
import TeacherSchedule from "./components/TeacherSchedule";
import SchoolCalendar from "./components/SchoolCalendar";
import CountdownWindow from "./components/CountdownWindow";
import CountdownEditWindow from "./components/CountdownEditWindow";
import DesktopBall from "./components/DesktopBall";
import ToolPanel from "./components/ToolPanel";

import { invoke } from "@tauri-apps/api/core";
import { loginTIM, getTIMGroups, setCachedTIMGroups } from "./utils/tim";

// Main App Component handling Login/Dashboard flow
function MainApp() {
  const [isLoggedIn, setIsLoggedIn] = useState(false);
  const [userInfo, setUserInfo] = useState<any>(null);

  const handleLoginSuccess = async (data: any) => {
    console.log('Login Success Data:', JSON.stringify(data));
    // Save token for API calls
    const token = data.token || data.data?.token || data.access_token || data.data?.access_token || '';
    if (token) localStorage.setItem('token', token);

    try {
      await invoke('resize_window');

      // Fetch full user info
      let phone = data.phone || data.data?.phone;
      let userId = data.id || data.data?.id;

      if (!phone && !userId && data.userinfo && Array.isArray(data.userinfo) && data.userinfo.length > 0) {
        phone = data.userinfo[0].phone;
        userId = data.userinfo[0].id;
      }

      if (phone || userId) {
        try {
          const infoResStr = await invoke<string>('get_user_info', {
            phone: phone,
            userId: userId ? String(userId) : null,
            token: token
          });
          const infoRes = JSON.parse(infoResStr);
          if (infoRes.data?.code === 200 && infoRes.data?.userinfo?.length > 0) {
            const fullInfo = infoRes.data.userinfo[0];
            console.log('Full User Info Fetched:', fullInfo);
            setUserInfo(fullInfo);
            setIsLoggedIn(true);

            if (fullInfo.teacher_unique_id) {
              localStorage.setItem('teacher_unique_id', fullInfo.teacher_unique_id);
              if (fullInfo.id_number) localStorage.setItem('id_number', fullInfo.id_number);
              if (fullInfo.name) localStorage.setItem('name', fullInfo.name);

              // Fetch UserSig for TIM
              try {
                const sig = await invoke<string>('get_user_sig', {
                  userId: fullInfo.teacher_unique_id
                });
                if (sig) {
                  console.log('UserSig fetched and saved');
                  localStorage.setItem('userSig', sig);

                  // Login to TIM and cache groups early so CreateClassGroupModal can use them
                  console.log('[App] Logging into TIM early...');
                  const timLoginSuccess = await loginTIM(fullInfo.teacher_unique_id, sig);
                  if (timLoginSuccess) {
                    console.log('[App] TIM login success, fetching groups for cache...');
                    const timGroups = await getTIMGroups();
                    setCachedTIMGroups(timGroups);
                    console.log('[App] TIM groups cached:', timGroups.length);
                  }
                }
              } catch (e) {
                console.error('Failed to fetch UserSig or login TIM:', e);
              }
              return;
            }
          }
        } catch (err) {
          console.error('Failed to fetch user info:', err);
        }
      }
    } catch (e) {
      console.error('Failed to resize window or fetch info:', e);
    }

    setUserInfo(data);
    setIsLoggedIn(true);
  };

  return (
    <div className="h-screen w-screen overflow-hidden text-ink-600 selection:bg-sage-500 selection:text-white">
      {!isLoggedIn ? (
        <Login onLoginSuccess={handleLoginSuccess} />
      ) : (
        <Dashboard userInfo={userInfo} />
      )}
    </div>
  );
}

// Root App Component dealing with Routing
function App() {
  return (
    <HashRouter>
      <Routes>
        <Route path="/" element={<MainApp />} />
        <Route path="/class/schedule/:groupclassId" element={<ClassScheduleWindow />} />
        <Route path="/class/chat/:groupclassId" element={<ClassChatWindow />} />
        <Route path="/intercom/:groupId" element={<IntercomWindow />} />
        <Route path="/chat/normal/:groupId" element={<NormalGroupChatWindow />} />
        <Route path="/file-box/:boxId" element={<DesktopFileBox />} />
        <Route path="/teacher-schedule" element={<TeacherSchedule />} />
        <Route path="/school-calendar" element={<SchoolCalendar />} />
        <Route path="/countdown" element={<CountdownWindow />} />
        <Route path="/countdown-edit" element={<CountdownEditWindow />} />
        <Route path="/desktop-ball" element={<DesktopBall />} />
        <Route path="/tool-panel" element={<ToolPanel />} />
      </Routes>
    </HashRouter>
  );
}

export default App;

