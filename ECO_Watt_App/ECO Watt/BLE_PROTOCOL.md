Echo Watt BLE Protocol

UUID

Service: 5f524c4e-0001-4a5b-9c1e-6f2b1a8d3c00

Status (READ + NOTIFY): 5f524c4e-0002-4a5b-9c1e-6f2b1a8d3c00

Control (WRITE): 5f524c4e-0003-4a5b-9c1e-6f2b1a8d3c00

Kontrol relay

{"relay":1,"state":true}

{"relay":1,"state":false}

{"command":"ALL_ON"}

{"command":"ALL_OFF"}

Countdown

Menyalakan relay 2 dan mematikannya setelah 15 menit:

{"timer":{"relay":2,"seconds":900}}

Membatalkan countdown relay 2:

{"timer":{"relay":2,"seconds":0}}

Countdown berjalan di ESP32 selama perangkat tetap mendapat daya.

Pengaturan

{
  "settings": {
    "disconnect_delay": 10,
    "connect_mode": "RESTORE_LAST"
  }
}

Nilai disconnect_delay: 3, 5, 10, atau 30 detik.

Nilai connect_mode:

ALL_ON: semua relay ON saat terhubung.

RESTORE_LAST: kembalikan status relay terakhir yang tersimpan.

STAY_OFF: tetap OFF sampai pengguna menekan kontrol manual.

Status ringkas

{
  "c": 1,
  "r": [1, 0, 1, 1],
  "t": [0, 899, 0, 0],
  "ad": 10,
  "cm": 1,
  "su": 1
}

c: status koneksi.

r: status Relay 1–4.

t: sisa countdown dalam detik untuk Relay 1–4.

ad: waktu auto-OFF setelah disconnect.

cm: mode koneksi (0=ALL_ON, 1=RESTORE_LAST, 2=STAY_OFF).

su: single-user aktif.