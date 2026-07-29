package main

import (
	"database/sql"
	"encoding/json"
	"fmt"
	"log"
	"net/http"
	"os"
	"time"

	_ "modernc.org/sqlite"
)

var db *sql.DB
var apiKey string

type eventRequest struct {
	Event     string `json:"event"`
	Timestamp string `json:"timestamp"`
}

func postEvent(w http.ResponseWriter, r *http.Request) {
	if r.Method != http.MethodPost {
		http.Error(w, "method not allowed", http.StatusMethodNotAllowed)
		return
	}

	if apiKey != "" && r.Header.Get("X-API-Key") != apiKey {
		http.Error(w, "unauthorized", http.StatusUnauthorized)
		return
	}

	var body eventRequest
	if err := json.NewDecoder(r.Body).Decode(&body); err != nil || body.Event == "" || body.Timestamp == "" {
		http.Error(w, "bad request: event and timestamp fields required", http.StatusBadRequest)
		return
	}

	if _, err := time.Parse(time.RFC3339, body.Timestamp); err != nil {
		http.Error(w, "bad request: timestamp must be RFC3339", http.StatusBadRequest)
		return
	}

	if _, err := db.Exec("INSERT INTO events (event_type, timestamp) VALUES (?, ?)", body.Event, body.Timestamp); err != nil {
		log.Printf("db insert: %v", err)
		http.Error(w, "internal error", http.StatusInternalServerError)
		return
	}

	w.Header().Set("Content-Type", "application/json")
	w.WriteHeader(http.StatusCreated)
	fmt.Fprintf(w, `{"status":"ok","timestamp":"%s"}`, body.Timestamp)
}

func initDB(path string) {
	var err error
	db, err = sql.Open("sqlite", path)
	if err != nil {
		log.Fatalf("open db: %v", err)
	}
	_, err = db.Exec(`CREATE TABLE IF NOT EXISTS events (
		id         INTEGER PRIMARY KEY AUTOINCREMENT,
		event_type TEXT    NOT NULL,
		timestamp  TEXT    NOT NULL
	)`)
	if err != nil {
		log.Fatalf("init db: %v", err)
	}
}

func main() {
	apiKey = os.Getenv("ZYNCOUNTER_API_KEY")

	dbPath := os.Getenv("ZYNCOUNTER_DB_PATH")
	if dbPath == "" {
		dbPath = "zyncounter.db"
	}

	port := os.Getenv("ZYNCOUNTER_PORT")
	if port == "" {
		port = "8080"
	}

	initDB(dbPath)

	http.HandleFunc("/events", postEvent)

	log.Printf("listening on :%s", port)
	log.Fatal(http.ListenAndServe(":"+port, nil))
}
