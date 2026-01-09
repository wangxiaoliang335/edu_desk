// Learn more about Tauri commands at https://tauri.app/develop/calling-rust/
use tauri::Manager;

#[tauri::command]
fn greet(name: &str) -> String {
    format!("Hello, {}! You've been greeted from Rust!", name)
}

#[tauri::command]
async fn login(phone: &str, password: &str) -> Result<String, String> {
    let client = reqwest::Client::new();
    let res = client.post("http://47.100.126.194:5000/login")
        .json(&serde_json::json!({
            "phone": phone,
            "password": password
        }))
        .send()
        .await
        .map_err(|e| e.to_string())?;

    let text = res.text().await.map_err(|e| e.to_string())?;
    Ok(text)
}

#[tauri::command]
async fn send_verification_code(phone: &str) -> Result<String, String> {
    let client = reqwest::Client::new();
    // Logic from RegisterDialog.cpp/ResetPwdDialog.cpp
    let res = client.post("http://47.100.126.194:5000/send_verification_code")
        .form(&serde_json::json!({
            "phone": phone
        }))
        .send()
        .await
        .map_err(|e| e.to_string())?;

    let text = res.text().await.map_err(|e| e.to_string())?;
    Ok(text)
}

#[tauri::command]
async fn register_account(phone: &str, password: &str, verification_code: &str) -> Result<String, String> {
    let client = reqwest::Client::new();
    // Logic from RegisterDialog.cpp
    let res = client.post("http://47.100.126.194:5000/register")
        .form(&serde_json::json!({
            "phone": phone,
            "password": password,
            "verification_code": verification_code
        }))
        .send()
        .await
        .map_err(|e| e.to_string())?;

    let text = res.text().await.map_err(|e| e.to_string())?;
    Ok(text)
}

#[tauri::command]
async fn reset_password(phone: &str, new_password: &str, verification_code: &str) -> Result<String, String> {
    let client = reqwest::Client::new();
    // Logic from ResetPwdDialog.cpp
    let res = client.post("http://47.100.126.194:5000/verify_and_set_password")
        .form(&serde_json::json!({
            "phone": phone,
            "new_password": new_password,
            "verification_code": verification_code
        }))
        .send()
        .await
        .map_err(|e| e.to_string())?;

    let text = res.text().await.map_err(|e| e.to_string())?;
    Ok(text)
}

#[tauri::command]
async fn resize_window(window: tauri::Window) {
    let _ = window.set_resizable(true);
    let _ = window.set_size(tauri::Size::Physical(tauri::PhysicalSize { width: 1280, height: 800 }));
    let _ = window.center();
}

#[tauri::command]
async fn get_course_schedule(class_id: &str, term: Option<&str>, token: &str) -> Result<String, String> {
    let client = reqwest::Client::new();
    let mut url = format!("http://47.100.126.194:5000/course-schedule?class_id={}", class_id);
    
    if let Some(t) = term {
        url.push_str(&format!("&term={}", t));
    }

    let res = client.get(&url)
        .header("Authorization", token) // Assuming token is passed directly or as Bearer? User said "Token: needed in header". Usually implies "Authorization". Qt code might clarify but let's try direct or Bearer. I'll use raw token first as Qt usually sends raw or specific format.
        .send()
        .await
        .map_err(|e| e.to_string())?;

    if res.status().is_success() {
        let text = res.text().await.map_err(|e| e.to_string())?;
        Ok(text)
    } else {
        Err(format!("Request failed with status: {}", res.status()))
    }
}

#[tauri::command]
async fn get_user_friends(id_card: &str, token: &str) -> Result<String, String> {
    println!("Backend: get_user_friends called with id_card: {}", id_card);
    let client = reqwest::Client::new();
    let url = format!("http://47.100.126.194:5000/friends?id_card={}", id_card);

    let res = client.get(&url)
        .header("Authorization", token)
        .send()
        .await
        .map_err(|e| e.to_string())?;

    let status = res.status();
    println!("Backend: get_user_friends status: {}", status);

    if status.is_success() {
        let text = res.text().await.map_err(|e| e.to_string())?;
        // println!("Backend: get_user_friends response: {}", text); // Optional: verbose
        Ok(text)
    } else {
        Err(format!("Request failed with status: {}", status))
    }
}

#[tauri::command]
async fn get_teacher_classes(teacher_unique_id: &str, token: &str) -> Result<String, String> {
    println!("Backend: get_teacher_classes called with teacher_unique_id: {}", teacher_unique_id);
    let client = reqwest::Client::new();
    let url = format!("http://47.100.126.194:5000/teachers/classes?teacher_unique_id={}", teacher_unique_id);

    let res = client.get(&url)
        .header("Authorization", token)
        .send()
        .await
        .map_err(|e| e.to_string())?;
    
    let status = res.status();
    println!("Backend: get_teacher_classes status: {}", status);

    if status.is_success() {
        let text = res.text().await.map_err(|e| e.to_string())?;
        Ok(text)
    } else {
        Err(format!("Request failed with status: {}", status))
    }
}

#[tauri::command]
async fn get_user_info(phone: Option<&str>, user_id: Option<&str>, token: &str) -> Result<String, String> {
    println!("Backend: get_user_info called. Phone: {:?}, UserId: {:?}", phone, user_id);
    let client = reqwest::Client::new();
    let mut url = "http://47.100.126.194:5000/userInfo?".to_string();
    
    if let Some(p) = phone {
        url.push_str(&format!("phone={}", p));
    } else if let Some(u) = user_id {
        url.push_str(&format!("userid={}", u));
    }

    let res = client.get(&url)
        .header("Authorization", token)
        .send()
        .await
        .map_err(|e| e.to_string())?;

    let status = res.status();
    println!("Backend: get_user_info status: {}", status);

    if status.is_success() {
        Ok(res.text().await.map_err(|e| e.to_string())?)
    } else {
        Err(format!("Request failed with status: {}", status))
    }
}

#[tauri::command]
async fn get_user_sig(user_id: &str) -> Result<String, String> {
    println!("Backend: get_user_sig called for user_id: {}", user_id);
    let client = reqwest::Client::new();
    let res = client.post("http://47.100.126.194:5000/getUserSig")
        .form(&serde_json::json!({
            "user_id": user_id
        }))
        .send()
        .await
        .map_err(|e| e.to_string())?;

    let status = res.status();
    println!("Backend: get_user_sig status: {}", status);

    if status.is_success() {
        let text = res.text().await.map_err(|e| e.to_string())?;
        // Parse the JSON to extract the user_sig field
        let json: serde_json::Value = serde_json::from_str(&text).map_err(|e| e.to_string())?;
        
        // C++ checks multiple fields: user_sig, usersig, sig
        let sig = json["data"]["user_sig"].as_str()
            .or_else(|| json["data"]["usersig"].as_str())
            .or_else(|| json["data"]["sig"].as_str())
            .or_else(|| json["user_sig"].as_str())
            .or_else(|| json["usersig"].as_str())
            .or_else(|| json["sig"].as_str());

        if let Some(s) = sig {
            println!("Backend: UserSig found");
            Ok(s.to_string())
        } else {
            println!("Backend: UserSig NOT found in response: {}", text);
            Err("UserSig not found in response".to_string())
        }
    } else {
        Err(format!("Request failed with status: {}", status))
    }
}

#[tauri::command]
async fn open_class_window(app: tauri::AppHandle, groupclass_id: String) -> Result<(), String> {
    println!("Backend: Opening class schedule window for ID: {}", groupclass_id);
    
    let window_label = format!("class_schedule_{}", groupclass_id);
    // Route to the new Schedule Window
    let url = format!("/class/schedule/{}", groupclass_id);

    if let Some(window) = app.get_webview_window(&window_label) {
        let _ = window.set_focus();
        return Ok(());
    }

    let builder = tauri::WebviewWindowBuilder::new(
        &app,
        window_label,
        tauri::WebviewUrl::App(url.into()),
    )
    .title("班级空间")
    .inner_size(1280.0, 800.0)
    .decorations(false)
    .transparent(true);

    match builder.build() {
        Ok(_) => Ok(()),
        Err(e) => Err(format!("Failed to create window: {}", e)),
    }
}

#[tauri::command]
async fn open_chat_window(app: tauri::AppHandle, groupclass_id: String) -> Result<(), String> {
    println!("Backend: Opening class chat window for ID: {}", groupclass_id);
    
    let window_label = format!("class_chat_{}", groupclass_id);
    // Route to the Chat Window
    let url = format!("/class/chat/{}", groupclass_id);

    if let Some(window) = app.get_webview_window(&window_label) {
        let _ = window.set_focus();
        return Ok(());
    }

    let builder = tauri::WebviewWindowBuilder::new(
        &app,
        window_label,
        tauri::WebviewUrl::App(url.into()),
    )
    .title("班级群")
    .inner_size(800.0, 600.0)
    .decorations(false)
    .transparent(true);

    match builder.build() {
        Ok(_) => Ok(()),
        Err(e) => Err(format!("Failed to create window: {}", e)),
    }
}

#[tauri::command]
async fn get_group_members(group_id: &str, token: &str) -> Result<String, String> {
    println!("Backend: get_group_members called for group_id: {}", group_id);
    let client = reqwest::Client::new();
    let url = format!("http://47.100.126.194:5000/groups/members?group_id={}", group_id);
    
    let res = client.get(&url)
        .header("Authorization", token)
        .send()
        .await
        .map_err(|e| e.to_string())?;

    let status = res.status();
    println!("Backend: get_group_members status: {}", status);

    if status.is_success() {
        let text = res.text().await.map_err(|e| e.to_string())?;
        Ok(text)
    } else {
        Err(format!("Request failed with status: {}", status))
    }
}



#[tauri::command]
async fn fetch_seat_map(class_id: String) -> Result<String, String> {
    println!("Backend: Fetching seat map for class_id: {}", class_id);
    let client = reqwest::Client::new();
    let url = "http://47.100.126.194:5000/seat-arrangement";
    
    let response = client
        .get(url)
        .query(&[("class_id", &class_id)])
        .send()
        .await
        .map_err(|e| e.to_string())?;

    let text = response.text().await.map_err(|e| e.to_string())?;
    Ok(text)
}

#[tauri::command]
async fn save_seat_map(class_id: String, seats: Vec<serde_json::Value>) -> Result<String, String> {
    println!("Backend: Saving seat map for class_id: {}", class_id);
    let client = reqwest::Client::new();
    let url = "http://47.100.126.194:5000/seat-arrangement/save";

    let response = client
        .post(url)
        .json(&serde_json::json!({
            "class_id": class_id,
            "seats": seats
        }))
        .send()
        .await
        .map_err(|e| e.to_string())?;
    
    let text = response.text().await.map_err(|e| e.to_string())?;
    Ok(text)
}

#[tauri::command]
async fn save_teach_subjects(group_id: String, user_id: String, teach_subjects: Vec<String>) -> Result<String, String> {
    println!("Backend: Saving teach subjects for group_id: {}", group_id);
    let client = reqwest::Client::new();
    let url = "http://47.100.126.194:5000/groups/member/teach-subjects";

    let response = client
        .post(url)
        .json(&serde_json::json!({
            "group_id": group_id,
            "user_id": user_id,
            "teach_subjects": teach_subjects
        }))
        .send()
        .await
        .map_err(|e| e.to_string())?;
    
    let text = response.text().await.map_err(|e| e.to_string())?;
    Ok(text)
}

#[cfg_attr(mobile, tauri::mobile_entry_point)]
pub fn run() {
    tauri::Builder::default()
    .plugin(tauri_plugin_opener::init())
        .invoke_handler(tauri::generate_handler![
            greet,
            login,
            send_verification_code,
            register_account,
            reset_password,
            resize_window,
            get_course_schedule,
            get_user_friends,
            get_teacher_classes,
            get_user_info,
            get_user_sig,
            get_group_members,
            open_class_window,
            open_chat_window,
            fetch_seat_map,
            save_seat_map,
            save_teach_subjects
        ])
        .run(tauri::generate_context!())
        .expect("error while running tauri application");
}
