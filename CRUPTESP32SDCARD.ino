#include <FS.h>
#include <SD.h>
#include <ESPAsyncWebServer.h>

#define SD_CS_PIN 5

File dataFile;

const char *ssid = "Pasti Dadi";
const char *password = "bejo0000";

struct Contact {
  int id;
  String nama;
  String alamat;
  String nomor_tlp;
};

AsyncWebServer server(80);

void setup() {
  Serial.begin(115200);

  if (!SD.begin(SD_CS_PIN)) {
    Serial.println("Gagal menginisialisasi SD Card!");
    return;
  }

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    String html = "<html><body>";
    html += "<h1>Esp 32 Crupt ADD/EDIT/DELETE SAVE SDCARD</h1>";
    html += "<table border='1'>";
    html += "<tr><th>ID</th><th>Nama</th><th>Alamat</th><th>Nomor Tlp</th><th>Action</th></tr>";

    File file = SD.open("/data.txt");
    if (file) {
      int id = 1;
      while (file.available()) {
        String line = file.readStringUntil('\n');
        Contact contact = parseContact(line);
        html += "<tr>";
        html += "<td>" + String(contact.id) + "</td>";
        html += "<td>" + contact.nama + "</td>";
        html += "<td>" + contact.alamat + "</td>";
        html += "<td>" + contact.nomor_tlp + "</td>";
        html += "<td><a href='/edit?id=" + String(contact.id) + "'>Edit</a> | <a href='/delete?id=" + String(contact.id) + "'>Delete</a></td>";
        html += "</tr>";
      }
      file.close();
    }

    html += "</table>";
    html += "<br><a href='/add'>Tambah Data</a>";
    html += "</body></html>";
    request->send(200, "text/html", html);
  });

  server.on("/add", HTTP_GET, [](AsyncWebServerRequest *request) {
    String html = "<html><body>";
    html += "<h1>Tambah Data</h1>";
    html += "<form action='/save' method='POST'>";
    html += "Nama: <input type='text' name='nama'><br>";
    html += "Alamat: <input type='text' name='alamat'><br>";
    html += "Nomor Tlp: <input type='text' name='nomor_tlp'><br>";
    html += "<input type='submit' value='Simpan'>";
    html += "</form>";
    html += "<br><a href='/'>Kembali ke Home</a>";
    html += "</body></html>";
    request->send(200, "text/html", html);
  });

  server.on("/save", HTTP_POST, [](AsyncWebServerRequest *request) {
    String nama = request->arg("nama");
    String alamat = request->arg("alamat");
    String nomor_tlp = request->arg("nomor_tlp");

    File file = SD.open("/data.txt", FILE_APPEND);
    if (file) {
      int id = countContacts() + 1;
      file.print(id);
      file.print(",");
      file.print(nama);
      file.print(",");
      file.print(alamat);
      file.print(",");
      file.println(nomor_tlp);
      file.close();
    }

    request->send(100, "text/plain", "Data berhasil di simpan");
    request->redirect("/");
  });

  server.on("/edit", HTTP_GET, [](AsyncWebServerRequest *request) {
    String idStr = request->arg("id");
    int id = idStr.toInt();

    String html = "<html><body>";
    html += "<h1>Edit Data</h1>";
    html += "<form action='/update' method='POST'>";
    html += "<input type='hidden' name='id' value='" + String(id) + "'>";
    html += "Nama: <input type='text' name='nama'><br>";
    html += "Alamat: <input type='text' name='alamat'><br>";
    html += "Nomor Tlp: <input type='text' name='nomor_tlp'><br>";
    html += "<input type='submit' value='Update'>";
    html += "</form>";
    html += "<br><a href='/'>Kembali ke Home</a>";
    html += "</body></html>";
    request->send(200, "text/html", html);
  });

  server.on("/update", HTTP_POST, [](AsyncWebServerRequest *request) {
    String idStr = request->arg("id");
    int id = idStr.toInt();

    String nama = request->arg("nama");
    String alamat = request->arg("alamat");
    String nomor_tlp = request->arg("nomor_tlp");

    updateContact(id, nama, alamat, nomor_tlp);

    request->send(100, "text/plain", "Data berhasil Di Update");
    request->redirect("/");
  });

  server.on("/delete", HTTP_GET, [](AsyncWebServerRequest *request) {
    String idStr = request->arg("id");
    int id = idStr.toInt();

    deleteContact(id);

    request->send(100, "text/plain", "Data berhasil di hapus");
    request->redirect("/");
  });

  server.begin();
}

void loop() {
  // Nothing to do here
}

Contact parseContact(String line) {
  Contact contact;
  int comma1 = line.indexOf(',');
  int comma2 = line.indexOf(',', comma1 + 1);
  int comma3 = line.indexOf(',', comma2 + 1);

  contact.id = line.substring(0, comma1).toInt();
  contact.nama = line.substring(comma1 + 1, comma2);
  contact.alamat = line.substring(comma2 + 1, comma3);
  contact.nomor_tlp = line.substring(comma3 + 1);

  return contact;
}

int countContacts() {
  int count = 0;
  File file = SD.open("/data.txt");
  if (file) {
    while (file.available()) {
      file.readStringUntil('\n');
      count++;
    }
    file.close();
  }
  return count;
}

void updateContact(int id, String nama, String alamat, String nomor_tlp) {
  File inputFile = SD.open("/data.txt");
  File tempFile = SD.open("/temp.txt", FILE_WRITE);

  if (inputFile && tempFile) {
    while (inputFile.available()) {
      String line = inputFile.readStringUntil('\n');
      Contact contact = parseContact(line);

      if (contact.id == id) {
        tempFile.print(id);
        tempFile.print(",");
        tempFile.print(nama);
        tempFile.print(",");
        tempFile.print(alamat);
        tempFile.print(",");
        tempFile.println(nomor_tlp);
      } else {
        tempFile.println(line);
      }
    }

    inputFile.close();
    tempFile.close();

    SD.remove("/data.txt");
    SD.rename("/temp.txt", "/data.txt");
  }
}

void deleteContact(int id) {
  File inputFile = SD.open("/data.txt");
  File tempFile = SD.open("/temp.txt", FILE_WRITE);

  if (inputFile && tempFile) {
    while (inputFile.available()) {
      String line = inputFile.readStringUntil('\n');
      Contact contact = parseContact(line);

      if (contact.id != id) {
        tempFile.println(line);
      }
    }

    inputFile.close();
    tempFile.close();

    SD.remove("/data.txt");
    SD.rename("/temp.txt", "/data.txt");
  }
}
