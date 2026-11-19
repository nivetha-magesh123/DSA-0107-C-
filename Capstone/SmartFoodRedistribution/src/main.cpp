#include "crow.h"
#include "database.hpp"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

static std::string value(const crow::json::rvalue& j, const char* key, const std::string& fallback = "") {
    if (!j.has(key)) return fallback;
    try { return j[key].s(); } catch (...) { return fallback; }
}
static int intValue(const crow::json::rvalue& j, const char* key, int fallback = 0) {
    if (!j.has(key)) return fallback;
    try { return static_cast<int>(j[key].i()); } catch (...) { return fallback; }
}
static double doubleValue(const crow::json::rvalue& j, const char* key, double fallback = 0) {
    if (!j.has(key)) return fallback;
    try { return j[key].d(); } catch (...) { try { return static_cast<double>(j[key].i()); } catch (...) { return fallback; } }
}
static crow::response errorResponse(int code, const std::string& message) {
    crow::json::wvalue out; out["success"] = false; out["message"] = message;
    return crow::response(code, out);
}
static crow::response successResponse(const std::string& message, int id = 0) {
    crow::json::wvalue out; out["success"] = true; out["message"] = message; if (id) out["id"] = id;
    return crow::response(200, out);
}
static bool hasText(const std::string& s) { return !s.empty() && s.find_first_not_of(" \t\r\n") != std::string::npos; }

int main() {
    try {
        Database database("food_system.db");
        database.initialize();
        sqlite3* db = database.handle();
        crow::SimpleApp app;

        CROW_ROUTE(app, "/")([] {
            crow::response res;
            res.set_static_file_info("web/index.html");
            return res;
        });
        CROW_ROUTE(app, "/<string>")([](const std::string& file) {
            if (file != "style.css" && file != "script.js") return crow::response(404);
            crow::response res; res.set_static_file_info("web/" + file); return res;
        });

        CROW_ROUTE(app, "/api/health")([] {
            crow::json::wvalue out; out["success"] = true; out["message"] = "Smart Food Redistribution API is running"; return out;
        });

        CROW_ROUTE(app, "/api/donors").methods(crow::HTTPMethod::POST)([db](const crow::request& req) {
            auto j = crow::json::load(req.body); if (!j) return errorResponse(400, "Invalid JSON");
            std::string name=value(j,"name"), phone=value(j,"phone"), email=value(j,"email"), location=value(j,"location"), type=value(j,"donor_type","Individual");
            if (!hasText(name) || !hasText(phone) || !hasText(location)) return errorResponse(400,"Name, phone and location are required");
            sqlite3_stmt* st=nullptr;
            const char* sql="INSERT INTO donors(name,phone,email,location,donor_type) VALUES(?,?,?,?,?)";
            if(sqlite3_prepare_v2(db,sql,-1,&st,nullptr)!=SQLITE_OK) return errorResponse(500,"Database error");
            sqlite3_bind_text(st,1,name.c_str(),-1,SQLITE_TRANSIENT); sqlite3_bind_text(st,2,phone.c_str(),-1,SQLITE_TRANSIENT); sqlite3_bind_text(st,3,email.c_str(),-1,SQLITE_TRANSIENT); sqlite3_bind_text(st,4,location.c_str(),-1,SQLITE_TRANSIENT); sqlite3_bind_text(st,5,type.c_str(),-1,SQLITE_TRANSIENT);
            int rc=sqlite3_step(st); int id=(int)sqlite3_last_insert_rowid(db); sqlite3_finalize(st);
            return rc==SQLITE_DONE ? successResponse("Donor registered successfully",id) : errorResponse(500,"Could not save donor");
        });

        CROW_ROUTE(app, "/api/receivers").methods(crow::HTTPMethod::POST)([db](const crow::request& req) {
            auto j=crow::json::load(req.body); if(!j) return errorResponse(400,"Invalid JSON");
            std::string name=value(j,"name"), phone=value(j,"phone"), location=value(j,"location"); int urgency=intValue(j,"urgency"), qty=intValue(j,"required_quantity"), people=intValue(j,"people_count"); double distance=doubleValue(j,"distance_km");
            if(!hasText(name)||!hasText(location)||urgency<1||urgency>10||qty<=0||distance<0) return errorResponse(400,"Enter name, location, urgency (1-10), non-negative distance and required quantity");
            sqlite3_stmt* st=nullptr; const char* sql="INSERT INTO receivers(name,phone,location,urgency,distance_km,required_quantity,people_count) VALUES(?,?,?,?,?,?,?)";
            if(sqlite3_prepare_v2(db,sql,-1,&st,nullptr)!=SQLITE_OK) return errorResponse(500,"Database error");
            sqlite3_bind_text(st,1,name.c_str(),-1,SQLITE_TRANSIENT); sqlite3_bind_text(st,2,phone.c_str(),-1,SQLITE_TRANSIENT); sqlite3_bind_text(st,3,location.c_str(),-1,SQLITE_TRANSIENT); sqlite3_bind_int(st,4,urgency); sqlite3_bind_double(st,5,distance); sqlite3_bind_int(st,6,qty); sqlite3_bind_int(st,7,people);
            int rc=sqlite3_step(st); int id=(int)sqlite3_last_insert_rowid(db); sqlite3_finalize(st);
            return rc==SQLITE_DONE ? successResponse("Receiver registered successfully",id) : errorResponse(500,"Could not save receiver");
        });

        CROW_ROUTE(app, "/api/donations").methods(crow::HTTPMethod::POST)([db](const crow::request& req) {
            auto j=crow::json::load(req.body); if(!j) return errorResponse(400,"Invalid JSON");
            int donorId=intValue(j,"donor_id"), qty=intValue(j,"quantity"); std::string food=value(j,"food_name"), type=value(j,"food_type"), prepared=value(j,"prepared_time"), expiry=value(j,"expiry_time"), pickup=value(j,"pickup_location");
            if(donorId<=0||qty<=0||!hasText(food)||!hasText(type)||!hasText(expiry)||!hasText(pickup)) return errorResponse(400,"Provide valid donor ID, food name/type, quantity, expiry and pickup location");
            sqlite3_stmt* st=nullptr; const char* sql="INSERT INTO food_donations(donor_id,food_name,food_type,quantity,prepared_time,expiry_time,pickup_location) VALUES(?,?,?,?,?,?,?)";
            if(sqlite3_prepare_v2(db,sql,-1,&st,nullptr)!=SQLITE_OK) return errorResponse(500,"Database error");
            sqlite3_bind_int(st,1,donorId); sqlite3_bind_text(st,2,food.c_str(),-1,SQLITE_TRANSIENT); sqlite3_bind_text(st,3,type.c_str(),-1,SQLITE_TRANSIENT); sqlite3_bind_int(st,4,qty); sqlite3_bind_text(st,5,prepared.c_str(),-1,SQLITE_TRANSIENT); sqlite3_bind_text(st,6,expiry.c_str(),-1,SQLITE_TRANSIENT); sqlite3_bind_text(st,7,pickup.c_str(),-1,SQLITE_TRANSIENT);
            int rc=sqlite3_step(st); int id=(int)sqlite3_last_insert_rowid(db); sqlite3_finalize(st);
            return rc==SQLITE_DONE ? successResponse("Food donation added to inventory",id) : errorResponse(500,"Could not save donation. Check that donor ID exists.");
        });

        CROW_ROUTE(app, "/api/donors").methods(crow::HTTPMethod::GET)([db] {
            crow::json::wvalue::list rows; sqlite3_stmt* st=nullptr; sqlite3_prepare_v2(db,"SELECT id,name,phone,email,location,donor_type FROM donors ORDER BY id DESC",-1,&st,nullptr);
            while(sqlite3_step(st)==SQLITE_ROW){ crow::json::wvalue r; r["id"]=sqlite3_column_int(st,0); for(int c=1;c<6;c++) r[c==1?"name":c==2?"phone":c==3?"email":c==4?"location":"donor_type"]=(const char*)sqlite3_column_text(st,c); rows.push_back(std::move(r)); } sqlite3_finalize(st); crow::json::wvalue out; out["items"]=std::move(rows); return out;
        });

        CROW_ROUTE(app, "/api/receivers").methods(crow::HTTPMethod::GET)([db] {
            crow::json::wvalue::list rows; sqlite3_stmt* st=nullptr; sqlite3_prepare_v2(db,"SELECT id,name,phone,location,urgency,distance_km,required_quantity,people_count FROM receivers ORDER BY id DESC",-1,&st,nullptr);
            while(sqlite3_step(st)==SQLITE_ROW){ crow::json::wvalue r; r["id"]=sqlite3_column_int(st,0); r["name"]=(const char*)sqlite3_column_text(st,1); r["phone"]=(const char*)sqlite3_column_text(st,2); r["location"]=(const char*)sqlite3_column_text(st,3); r["urgency"]=sqlite3_column_int(st,4); r["distance_km"]=sqlite3_column_double(st,5); r["required_quantity"]=sqlite3_column_int(st,6); r["people_count"]=sqlite3_column_int(st,7); rows.push_back(std::move(r)); } sqlite3_finalize(st); crow::json::wvalue out; out["items"]=std::move(rows); return out;
        });

        CROW_ROUTE(app, "/api/inventory").methods(crow::HTTPMethod::GET)([db] {
            crow::json::wvalue::list rows; sqlite3_stmt* st=nullptr;
            const char* sql="SELECT f.id,f.food_name,f.food_type,f.quantity,f.expiry_time,f.pickup_location,f.status,d.name FROM food_donations f JOIN donors d ON d.id=f.donor_id WHERE f.status='Available' AND datetime(replace(f.expiry_time,'T',' ')) > datetime('now','localtime') ORDER BY f.id DESC";
            sqlite3_prepare_v2(db,sql,-1,&st,nullptr); while(sqlite3_step(st)==SQLITE_ROW){ crow::json::wvalue r; r["id"]=sqlite3_column_int(st,0); r["food_name"]=(const char*)sqlite3_column_text(st,1); r["food_type"]=(const char*)sqlite3_column_text(st,2); r["quantity"]=sqlite3_column_int(st,3); r["expiry_time"]=(const char*)sqlite3_column_text(st,4); r["pickup_location"]=(const char*)sqlite3_column_text(st,5); r["status"]=(const char*)sqlite3_column_text(st,6); r["donor_name"]=(const char*)sqlite3_column_text(st,7); rows.push_back(std::move(r)); } sqlite3_finalize(st); crow::json::wvalue out; out["items"]=std::move(rows); return out;
        });

        CROW_ROUTE(app, "/api/match/<int>").methods(crow::HTTPMethod::POST)([db](int foodId) {
            sqlite3_stmt* food=nullptr; const char* fsql="SELECT quantity,expiry_time,status FROM food_donations WHERE id=?"; sqlite3_prepare_v2(db,fsql,-1,&food,nullptr); sqlite3_bind_int(food,1,foodId);
            if(sqlite3_step(food)!=SQLITE_ROW){sqlite3_finalize(food);return errorResponse(404,"Food donation not found");}
            int foodQty=sqlite3_column_int(food,0); std::string expiry=(const char*)sqlite3_column_text(food,1); std::string status=(const char*)sqlite3_column_text(food,2); sqlite3_finalize(food);
            if(status!="Available") return errorResponse(409,"This donation is not available for matching");
            sqlite3_stmt* check=nullptr; sqlite3_prepare_v2(db,"SELECT datetime(replace(?,'T',' ')) > datetime('now','localtime')",-1,&check,nullptr); sqlite3_bind_text(check,1,expiry.c_str(),-1,SQLITE_TRANSIENT); bool fresh=sqlite3_step(check)==SQLITE_ROW && sqlite3_column_int(check,0)==1; sqlite3_finalize(check); if(!fresh)return errorResponse(400,"This food has expired or has an invalid expiry date");
            sqlite3_stmt* receivers=nullptr; sqlite3_prepare_v2(db,"SELECT id,name,urgency,distance_km,required_quantity FROM receivers WHERE required_quantity<=? ORDER BY id",-1,&receivers,nullptr); sqlite3_bind_int(receivers,1,foodQty);
            int bestId=0,bestQty=0; std::string bestName; double bestScore=-1;
            while(sqlite3_step(receivers)==SQLITE_ROW){int id=sqlite3_column_int(receivers,0); std::string name=(const char*)sqlite3_column_text(receivers,1); int urgency=sqlite3_column_int(receivers,2); double distance=sqlite3_column_double(receivers,3); int required=sqlite3_column_int(receivers,4); double u=urgency; double l=std::max(0.0,10.0-distance); double q=std::min(10.0,(double)foodQty/required*10.0); double score=0.5*u+0.3*l+0.2*q; if(score>bestScore){bestScore=score;bestId=id;bestName=name;bestQty=required;}}
            sqlite3_finalize(receivers); if(!bestId)return errorResponse(404,"No eligible receiver found. Check receiver requirements and food quantity.");
            sqlite3_exec(db,"BEGIN IMMEDIATE TRANSACTION",nullptr,nullptr,nullptr);
            sqlite3_stmt* ins=nullptr; sqlite3_prepare_v2(db,"INSERT INTO distributions(food_id,receiver_id,allocated_quantity) VALUES(?,?,?)",-1,&ins,nullptr); sqlite3_bind_int(ins,1,foodId); sqlite3_bind_int(ins,2,bestId); sqlite3_bind_int(ins,3,bestQty); int irc=sqlite3_step(ins); sqlite3_finalize(ins);
            sqlite3_stmt* upd=nullptr; sqlite3_prepare_v2(db,"UPDATE food_donations SET status='Allocated' WHERE id=? AND status='Available'",-1,&upd,nullptr); sqlite3_bind_int(upd,1,foodId); int urc=sqlite3_step(upd); int changed=sqlite3_changes(db); sqlite3_finalize(upd);
            if(irc!=SQLITE_DONE||urc!=SQLITE_DONE||changed!=1){sqlite3_exec(db,"ROLLBACK",nullptr,nullptr,nullptr);return errorResponse(409,"Allocation could not be completed; refresh inventory and try again");}
            sqlite3_exec(db,"COMMIT",nullptr,nullptr,nullptr);
            crow::json::wvalue out; out["success"]=true; out["message"]="Best receiver matched successfully"; out["receiver_id"]=bestId; out["receiver_name"]=bestName; out["allocated_quantity"]=bestQty; out["score"]=bestScore; return crow::response(200,out);
        });

        CROW_ROUTE(app, "/api/tracking").methods(crow::HTTPMethod::GET)([db] {
            crow::json::wvalue::list rows; sqlite3_stmt* st=nullptr; const char* sql="SELECT x.id,f.food_name,x.allocated_quantity,r.name,x.status,x.allocation_date FROM distributions x JOIN food_donations f ON f.id=x.food_id JOIN receivers r ON r.id=x.receiver_id ORDER BY x.id DESC"; sqlite3_prepare_v2(db,sql,-1,&st,nullptr);
            while(sqlite3_step(st)==SQLITE_ROW){crow::json::wvalue r;r["id"]=sqlite3_column_int(st,0);r["food_name"]=(const char*)sqlite3_column_text(st,1);r["quantity"]=sqlite3_column_int(st,2);r["receiver_name"]=(const char*)sqlite3_column_text(st,3);r["status"]=(const char*)sqlite3_column_text(st,4);r["date"]=(const char*)sqlite3_column_text(st,5);rows.push_back(std::move(r));}sqlite3_finalize(st);crow::json::wvalue out;out["items"]=std::move(rows);return out;
        });

        CROW_ROUTE(app, "/api/tracking/<int>").methods(crow::HTTPMethod::PATCH)([db](int id,const crow::request& req){auto j=crow::json::load(req.body);if(!j)return errorResponse(400,"Invalid JSON");std::string status=value(j,"status");std::vector<std::string> allowed={"Allocated","Picked Up","In Transit","Completed","Cancelled"};if(std::find(allowed.begin(),allowed.end(),status)==allowed.end())return errorResponse(400,"Invalid status");sqlite3_stmt* st=nullptr;sqlite3_prepare_v2(db,"UPDATE distributions SET status=?,updated_at=datetime('now','localtime') WHERE id=?",-1,&st,nullptr);sqlite3_bind_text(st,1,status.c_str(),-1,SQLITE_TRANSIENT);sqlite3_bind_int(st,2,id);int rc=sqlite3_step(st);int changed=sqlite3_changes(db);sqlite3_finalize(st);if(rc!=SQLITE_DONE||!changed)return errorResponse(404,"Distribution record not found");return successResponse("Distribution status updated");});

        CROW_ROUTE(app, "/api/stats").methods(crow::HTTPMethod::GET)([db]{crow::json::wvalue out;auto count=[db](const char* sql){sqlite3_stmt* st=nullptr;sqlite3_prepare_v2(db,sql,-1,&st,nullptr);int n=sqlite3_step(st)==SQLITE_ROW?sqlite3_column_int(st,0):0;sqlite3_finalize(st);return n;};out["donors"]=count("SELECT COUNT(*) FROM donors");out["receivers"]=count("SELECT COUNT(*) FROM receivers");out["donations"]=count("SELECT COUNT(*) FROM food_donations");out["available_food"]=count("SELECT COALESCE(SUM(quantity),0) FROM food_donations WHERE status='Available' AND datetime(replace(expiry_time,'T',' '))>datetime('now','localtime')");out["allocated_food"]=count("SELECT COALESCE(SUM(allocated_quantity),0) FROM distributions WHERE status!='Cancelled'");out["completed"]=count("SELECT COUNT(*) FROM distributions WHERE status='Completed'");return out;});

        std::cout << "Smart Food Redistribution System running at http://localhost:18080" << std::endl;
        app.port(18080).multithreaded().run();
    } catch (const std::exception& e) {
        std::cerr << "Startup error: " << e.what() << std::endl;
        return 1;
    }
}
