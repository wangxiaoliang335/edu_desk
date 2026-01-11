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

#[tauri::command]
async fn toggle_group_intercom(group_id: String, enable: bool) -> Result<String, String> {
    println!("Backend: Toggling intercom for group_id: {}, enable: {}", group_id, enable);
    let client = reqwest::Client::new();
    let url = "http://47.100.126.194:5000/groups/intercom/toggle";

    let response = client
        .post(url)
        .json(&serde_json::json!({
            "group_id": group_id,
            "enable_intercom": enable
        }))
        .send()
        .await
        .map_err(|e| e.to_string())?;
    
    let text = response.text().await.map_err(|e| e.to_string())?;
    Ok(text)
}

#[tauri::command]
async fn fetch_temp_room(group_id: String) -> Result<String, String> {
    println!("Backend: Fetching temp room for group_id: {}", group_id);
    let client = reqwest::Client::new();
    let url = "http://47.100.126.194:5000/temp_rooms/query";

    let response = client
        .post(url)
        .json(&serde_json::json!({
            "group_ids": [group_id]
        }))
        .send()
        .await
        .map_err(|e| e.to_string())?;
    
    let text = response.text().await.map_err(|e| e.to_string())?;
    Ok(text)
}

#[tauri::command]
async fn open_intercom_window(app: tauri::AppHandle, group_id: String) -> Result<(), String> {
    println!("Backend: Opening intercom window for ID: {}", group_id);
    
    let window_label = format!("intercom_{}", group_id);
    let url = format!("/intercom/{}", group_id);

    if let Some(window) = app.get_webview_window(&window_label) {
        let _ = window.set_focus();
        return Ok(());
    }

    let builder = tauri::WebviewWindowBuilder::new(
        &app,
        window_label,
        tauri::WebviewUrl::App(url.into()),
    )
    .title("对讲")
    .inner_size(800.0, 500.0) 
    .decorations(false)
    .transparent(true);

    match builder.build() {
        Ok(_) => Ok(()),
        Err(e) => Err(format!("Failed to create window: {}", e)),
    }
}

#[tauri::command]
async fn fetch_duty_roster(group_id: String) -> Result<String, String> {
    println!("Backend: Fetching duty roster for group_id: {}", group_id);
    let client = reqwest::Client::new();
    let url = format!("http://47.100.126.194:5000/duty-roster?group_id={}", group_id);
    let response = client.get(&url).send().await.map_err(|e| e.to_string())?;
    let text = response.text().await.map_err(|e| e.to_string())?;
    Ok(text)
}

#[tauri::command]
async fn save_duty_roster(group_id: String, rows: Vec<Vec<String>>, requirement_row_index: i32) -> Result<String, String> {
    println!("Backend: Saving duty roster for group_id: {}", group_id);
    let client = reqwest::Client::new();
    let url = "http://47.100.126.194:5000/duty-roster";
    let response = client.post(url)
        .json(&serde_json::json!({
            "group_id": group_id,
            "rows": rows,
            "requirement_row_index": requirement_row_index
        }))
        .send().await.map_err(|e| e.to_string())?;
    let text = response.text().await.map_err(|e| e.to_string())?;
    Ok(text)
}

#[tauri::command]
async fn fetch_class_wallpapers(group_id: String) -> Result<String, String> {
    println!("Backend: Fetching class wallpapers for group_id: {}", group_id);
    let client = reqwest::Client::new();
    let url = format!("http://47.100.126.194:5000/class-wallpapers?group_id={}", group_id);
    let response = client.get(&url).send().await.map_err(|e| e.to_string())?;
    let text = response.text().await.map_err(|e| e.to_string())?;
    Ok(text)
}

#[tauri::command]
async fn fetch_wallpaper_library() -> Result<String, String> {
    println!("Backend: Fetching wallpaper library");
    let client = reqwest::Client::new();
    let url = "http://47.100.126.194:5000/wallpaper-library";
    let response = client.get(url).send().await.map_err(|e| e.to_string())?;
    let text = response.text().await.map_err(|e| e.to_string())?;
    Ok(text)
}

#[tauri::command]
async fn set_class_wallpaper(group_id: String, wallpaper_id: i32) -> Result<String, String> {
    println!("Backend: Setting class wallpaper group_id: {}, wallpaper_id: {}", group_id, wallpaper_id);
    let client = reqwest::Client::new();
    let url = "http://47.100.126.194:5000/class-wallpapers/set-current";
    let response = client.post(url)
        .json(&serde_json::json!({
            "group_id": group_id,
            "wallpaper_id": wallpaper_id
        }))
        .send().await.map_err(|e| e.to_string())?;
    let text = response.text().await.map_err(|e| e.to_string())?;
    Ok(text)
}

const API_BASE_URL: &str = "http://47.100.126.194:5000/";

#[tauri::command]
async fn download_wallpaper(group_id: String, wallpaper_id: i32) -> String {
    let client = reqwest::Client::new();
    let url = format!("{}class-wallpapers/download", API_BASE_URL);
    let params = serde_json::json!({
        "group_id": group_id,
        "wallpaper_id": wallpaper_id
    });

    match client.post(&url).json(&params).send().await {
        Ok(res) => {
            match res.text().await {
                Ok(text) => text,
                Err(e) => format!("{{\"code\": 500, \"message\": \"Failed to read response: {}\"}}", e)
            }
        },
        Err(e) => format!("{{\"code\": 500, \"message\": \"Request failed: {}\"}}", e)
    }
}

#[tauri::command]
async fn upload_wallpaper(group_id: String, image_data: Vec<u8>, mime_type: String) -> String {
    let client = reqwest::Client::new();
    let url = format!("{}groups/wallpapers/upload", API_BASE_URL);

    // Create a Part from the bytes
    // We guess filename based on mime or just use generic
    let file_name = match mime_type.as_str() {
        "image/png" => "wallpaper.png",
        "image/jpeg" => "wallpaper.jpg",
        "image/gif" => "wallpaper.gif",
        _ => "wallpaper.bin",
    };

    let part = reqwest::multipart::Part::bytes(image_data)
        .file_name(file_name.to_string())
        .mime_str(&mime_type).unwrap();

    let form = reqwest::multipart::Form::new()
        .text("group_id", group_id)
        .part("file", part);

    match client.post(&url).multipart(form).send().await {
        Ok(res) => {
            match res.text().await {
                Ok(text) => text,
                Err(e) => format!("{{\"code\": 500, \"message\": \"Failed to read response: {}\"}}", e)
            }
        },
        Err(e) => format!("{{\"code\": 500, \"message\": \"Request failed: {}\"}}", e)
    }
}

#[tauri::command]
async fn save_student_score_sheet(
    class_id: String,
    term: String,
    exam_name: String,
    scores: Vec<serde_json::Value>,
    fields: Vec<serde_json::Value>
) -> String {
    let client = reqwest::Client::new();
    let url = format!("{}student-scores/save", API_BASE_URL);
    
    let params = serde_json::json!({
        "class_id": class_id,
        "term": term,
        "exam_name": exam_name,
        "operation_mode": "replace",
        "scores": scores,
        "fields": fields
    });

    match client.post(&url).json(&params).send().await {
        Ok(res) => {
            match res.text().await {
                Ok(text) => text,
                Err(e) => format!("{{\"code\": 500, \"message\": \"Failed to read response: {}\"}}", e)
            }
        },
        Err(e) => format!("{{\"code\": 500, \"message\": \"Request failed: {}\"}}", e)
    }
}

#[tauri::command]
async fn save_course_schedule(class_id: String, term: String, days: Vec<String>, times: Vec<String>, cells: serde_json::Value, token: String) -> Result<String, String> {
    println!("Backend: Saving course schedule for class_id: {}", class_id);
    let client = reqwest::Client::new();
    let url = "http://47.100.126.194:5000/course-schedule/save"; 

    let response = client.post(url)
        .header("Authorization", token)
        .json(&serde_json::json!({
            "class_id": class_id,
            "term": term,
            "days": days,
            "times": times,
            "cells": cells
        }))
        .send().await.map_err(|e| e.to_string())?;

    let text = response.text().await.map_err(|e| e.to_string())?;
    Ok(text)
}

#[tauri::command]
async fn get_classes_by_prefix(prefix: String) -> Result<String, String> {
    println!("Backend: get_classes_by_prefix called with prefix: {}", prefix);
    let client = reqwest::Client::new();
    let url = format!("{}{}", API_BASE_URL, "getClassesByPrefix");

    let response = client.post(&url)
        .json(&serde_json::json!({
            "prefix": prefix
        }))
        .send()
        .await
        .map_err(|e| e.to_string())?;

    let text = response.text().await.map_err(|e| e.to_string())?;
    Ok(text)
}

#[tauri::command]
async fn search_classes(keyword: String, school_id: Option<String>) -> Result<String, String> {
    println!("Backend: search_classes called with keyword: {}, school_id: {:?}", keyword, school_id);
    let client = reqwest::Client::new();
    let mut url = format!("{}classes/search?class_code={}", API_BASE_URL, keyword);

    if let Some(sid) = school_id {
        if !sid.is_empty() {
            url.push_str(&format!("&schoolid={}", sid));
        }
    }

    let response = client.get(&url)
        .send()
        .await
        .map_err(|e| e.to_string())?;

    let text = response.text().await.map_err(|e| e.to_string())?;
    Ok(text)
}

#[tauri::command]
async fn search_teachers(keyword: String) -> Result<String, String> {
    println!("Backend: search_teachers called with keyword: {}", keyword);
    let client = reqwest::Client::new();
    let url = format!("{}teachers/search", API_BASE_URL);
    
    // Determine if keyword looks like ID or Name (Simple logic: check if purely numeric/alphanumeric with special chars vs chinese)
    // Rust doesn't have easy "containsChinese" regex without crate, but we can pass generic params or just try one.
    // The python API likely handles query params manually.
    // Logic from SearchDialog.h: if looksLikeId -> teacher_unique_id else name.
    
    let is_id = keyword.chars().all(|c| c.is_ascii_alphanumeric() || c == '-' || c == '_');
    
    let query_param = if is_id {
        format!("teacher_unique_id={}", keyword)
    } else {
        format!("name={}", keyword)
    };
    
    let full_url = format!("{}?{}", url, query_param);

    let response = client.get(&full_url)
        .send()
        .await
        .map_err(|e| e.to_string())?;

    let text = response.text().await.map_err(|e| e.to_string())?;
    Ok(text)
}

#[tauri::command]
async fn search_class_groups(keyword: String) -> Result<String, String> {
    println!("Backend: search_class_groups called with keyword: {}", keyword);
    let client = reqwest::Client::new();
    let url = format!("{}groups/search", API_BASE_URL);
    
    let is_id = keyword.chars().all(|c| c.is_ascii_alphanumeric() || c == '-' || c == '_');
    
    let query_param = if is_id {
        format!("group_id={}", keyword)
    } else {
        format!("group_name={}", keyword)
    };
    
    let full_url = format!("{}?{}", url, query_param);

    let response = client.get(&full_url)
        .send()
        .await
        .map_err(|e| e.to_string())?;

    let text = response.text().await.map_err(|e| e.to_string())?;
    Ok(text)
}

#[tauri::command]
async fn join_class_group_request(group_id: String, user_id: String, user_name: String, reason: String) -> Result<String, String> {
    println!("Backend: join_class_group_request called. Group: {}, User: {}", group_id, user_id);
    let client = reqwest::Client::new();
    let url = format!("{}groups/join", API_BASE_URL); 

    let response = client.post(&url)
        .json(&serde_json::json!({
            "group_id": group_id,
            "user_id": user_id,
            "user_name": user_name,
            "reason": reason
        }))
        .send()
        .await
        .map_err(|e| e.to_string())?;

    let text = response.text().await.map_err(|e| e.to_string())?;
    Ok(text)
}

#[tauri::command]
async fn join_class(teacher_unique_id: String, class_code: String) -> Result<String, String> {
    println!("Backend: join_class called. Teacher: {}, ClassCode: {}", teacher_unique_id, class_code);
    let client = reqwest::Client::new();
    let url = format!("{}teachers/classes/add", API_BASE_URL);

    let response = client.post(&url)
        .json(&serde_json::json!({
            "teacher_unique_id": teacher_unique_id,
            "class_code": class_code,
            "role": "teacher"
        }))
        .send()
        .await
        .map_err(|e| e.to_string())?;

    let text = response.text().await.map_err(|e| e.to_string())?;
    Ok(text)
}

// Helper to fetch UserSig internally
async fn fetch_user_sig_internal(user_id: &str) -> Result<String, String> {
    let client = reqwest::Client::new();
    let url = format!("{}getUserSig", API_BASE_URL);
    let res = client.post(&url)
        .form(&serde_json::json!({ "user_id": user_id }))
        .send().await.map_err(|e| e.to_string())?;

    if res.status().is_success() {
        let text = res.text().await.map_err(|e| e.to_string())?;
        let json: serde_json::Value = serde_json::from_str(&text).map_err(|e| e.to_string())?;
        let sig = json["data"]["user_sig"].as_str()
            .or_else(|| json["data"]["usersig"].as_str())
            .or_else(|| json["data"]["sig"].as_str())
            .or_else(|| json["user_sig"].as_str())
            .or_else(|| json["usersig"].as_str())
            .or_else(|| json["sig"].as_str());
        
        if let Some(s) = sig {
            Ok(s.to_string())
        } else {
            Err("UserSig not found".to_string())
        }
    } else {
        Err(format!("Request failed: {}", res.status()))
    }
}

#[tauri::command]
async fn create_group_tim(owner_id: String, group_name: String, group_type: String) -> Result<String, String> {
    println!("Backend: create_group_tim called. Owner: {}, Name: {}, Type: {}", owner_id, group_name, group_type);
    
    // 1. Get UserSig
    let sig = fetch_user_sig_internal(&owner_id).await?;
    
    // 2. Construct TIM URL
    let sdk_app_id = 1600111046;
    let random = std::time::SystemTime::now().duration_since(std::time::UNIX_EPOCH).unwrap().as_secs();
    let url = format!("https://console.tim.qq.com/v4/group_open_http_svc/create_group?sdkappid={}&identifier={}&usersig={}&random={}&contenttype=json", 
        sdk_app_id, owner_id, sig, random);

    // 3. Construct Body
    let client = reqwest::Client::new();
    let response = client.post(&url)
        .json(&serde_json::json!({
            "Name": group_name,
            "Type": group_type, 
            "Owner_Account": owner_id,
            "GroupConfig": {
                "MaxMemberCount": 2000,
                "ApplyJoinOption": "FreeAccess"
            },
            "MemberList": [
                { "Member_Account": owner_id }
            ]
        }))
        .send()
        .await
        .map_err(|e| e.to_string())?;
        
    let text = response.text().await.map_err(|e| e.to_string())?;
    Ok(text)
}

#[tauri::command]
async fn remove_friend(teacher_unique_id: String, friend_teacher_unique_id: String) -> Result<String, String> {
    println!("Backend: remove_friend called. userId: {}, friendId: {}", teacher_unique_id, friend_teacher_unique_id);
    let client = reqwest::Client::new();
    let url = "http://47.100.126.194:5000/friends/remove";
    
    let res = client.post(url)
        .json(&serde_json::json!({
             "teacher_unique_id": teacher_unique_id,
             "friend_teacher_unique_id": friend_teacher_unique_id
        }))
        .send()
        .await
        .map_err(|e| e.to_string())?;

    let status = res.status();
    let text = res.text().await.map_err(|e| e.to_string())?;

    if status.is_success() {
        Ok(text)
    } else {
        Err(format!("Request failed with status: {}. Body: {}", status, text))
    }
}

#[tauri::command]
async fn leave_class(teacher_unique_id: String, class_code: String) -> Result<String, String> {
    println!("Backend: leave_class called. userId: {}, classCode: {}", teacher_unique_id, class_code);
    let client = reqwest::Client::new();
    let url = "http://47.100.126.194:5000/teachers/classes/remove";
    
    let res = client.post(url)
        .json(&serde_json::json!({
             "teacher_unique_id": teacher_unique_id,
             "class_code": class_code
        }))
        .send()
        .await
        .map_err(|e| e.to_string())?;

    let status = res.status();
    let text = res.text().await.map_err(|e| e.to_string())?;

    if status.is_success() {
        Ok(text)
    } else {
        Err(format!("Request failed with status: {}. Body: {}", status, text))
    }
}

#[tauri::command]
async fn exit_app() {
    std::process::exit(0);
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
            save_course_schedule,
            get_user_friends,
            get_teacher_classes,
            get_user_info,
            get_user_sig, 
            open_class_window, 
            open_chat_window, 
            get_group_members,
            fetch_seat_map,
            save_seat_map,
            save_teach_subjects,
            toggle_group_intercom,
            fetch_temp_room,
            open_intercom_window,
            fetch_duty_roster,
            save_duty_roster,
            fetch_class_wallpapers,
            fetch_wallpaper_library,
            set_class_wallpaper,
            download_wallpaper,
            upload_wallpaper,
            save_student_score_sheet,
            get_classes_by_prefix,
            search_classes,
            search_teachers,
            search_class_groups,
            join_class_group_request,
            create_group_tim,

            join_class,
            remove_friend,
            leave_class,
            exit_app
        ])
        .run(tauri::generate_context!())
        .expect("error while running tauri application");
}
