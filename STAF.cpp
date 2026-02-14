#include <iostream>
#include "httplib.h"
#include <sqlite3.h>
#include <fstream>
#include <sstream>

// --- Fonction pour insérer une annonce ---
void insertAnnonce(const std::string& titre, const std::string& desc,
                   const std::string& tel, const std::string& paiement,
                   const std::string& fichier) {
    sqlite3* db;
    sqlite3_open("annonces.db", &db);

    const char* sql = "INSERT INTO annonces (titre, description, telephone, paiement, fichier, valide) VALUES (?, ?, ?, ?, ?, 0);";
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);

    sqlite3_bind_text(stmt, 1, titre.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, desc.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, tel.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, paiement.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5, fichier.c_str(), -1, SQLITE_TRANSIENT);

    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    sqlite3_close(db);
}

// --- Fonction pour valider une annonce ---
void validerAnnonce(int id) {
    sqlite3* db;
    sqlite3_open("annonces.db", &db);
    const char* sql = "UPDATE annonces SET valide = 1 WHERE id = ?;";
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    sqlite3_bind_int(stmt, 1, id);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    sqlite3_close(db);
}

// --- Fonction pour afficher annonces validées ---
std::string getAnnoncesValidees() {
    sqlite3* db;
    sqlite3_open("annonces.db", &db);
    const char* sql = "SELECT titre, description, telephone, paiement, fichier FROM annonces WHERE valide = 1;";
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);

    std::ostringstream html;
    html << "<h2>Annonces validées STAF</h2>";

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        std::string titre = (const char*)sqlite3_column_text(stmt, 0);
        std::string desc = (const char*)sqlite3_column_text(stmt, 1);
        std::string tel = (const char*)sqlite3_column_text(stmt, 2);
        std::string paiement = (const char*)sqlite3_column_text(stmt, 3);
        std::string fichier = (const char*)sqlite3_column_text(stmt, 4);

        html << "<div style='border:1px solid #ccc; margin:10px; padding:10px;'>";
        html << "<h3>" << titre << "</h3>";
        html << "<p>" << desc << "</p>";
        html << "<p><b>Téléphone:</b> " << tel << "</p>";
        html << "<p><b>Paiement:</b> " << paiement << "</p>";
        html << "<p><b>Contactez Said (admin):</b> +212662106466</p>";

        if (!fichier.empty()) {
            if (fichier.find(".jpg") != std::string::npos || fichier.find(".png") != std::string::npos) {
                html << "<img src='/" << fichier << "' width='200'><br>";
            } else if (fichier.find(".mp4") != std::string::npos) {
                html << "<video width='320' controls><source src='/" << fichier << "' type='video/mp4'></video><br>";
            }
        }
        html << "</div>";
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return html.str();
}

// --- MAIN ---
int main() {
    httplib::Server svr;

    // Page d’accueil
    svr.Get("/", [](const httplib::Request&, httplib::Response& res) {
        std::string html = R"(
            <html><head><meta charset="UTF-8"><title>STAF - AutoImmobilier</title></head>
            <body style="font-family:Arial; background:#f4f4f4; text-align:center;">
                <header style="background:#333; color:white; padding:20px;">
                    <h1>🚗🏠 Bienvenue sur STAF - AutoImmobilier</h1>
                </header>
                <div style="margin:50px; padding:20px; background:white; border-radius:10px;">
                    <h2>Plateforme gratuite d'achat et vente</h2>
                    <a href="/deposer">➡️ Déposer une annonce</a><br>
                    <a href="/annonces">📋 Voir les annonces validées</a><br>
                    <a href="/admin">🔑 Administration</a>
                </div>
            </body></html>
        )";
        res.set_content(html, "text/html");
    });

    // Déposer une annonce
    svr.Get("/deposer", [](const httplib::Request&, httplib::Response& res) {
        std::string html = R"(
            <html><head><meta charset="UTF-8"><title>Déposer</title></head>
            <body style="font-family:Arial;>
                <h2>Déposer votre bien sur STAF</h2>
                <p><b>Contactez directementl'administrateur Said :</b> +212662106466</p>
                <form action="/save" method="post" enctype="multipart/form-data">
                    Titre: <input type="text" name="titre"><br>
                    Description: <textarea name="desc"></textarea><br>
                    Téléphone: <input type="text" name="tel"><br>
                    Paiement: <input type="text" name="paiement"><br>
                    Fichier (photo/vidéo): <input type="file" name="fichier"><br>
                    <input type="submit" value="Envoyer">
                </form>
            </body></html>
        )";
        res.set_content(html, "text/html");
    });

    // Sauvegarde annonce
    svr.Post("/save", [](const httplib::Request& req, httplib::Response& res) {
        auto titre = req.get_param_value("titre");
        auto desc = req.get_param_value("desc");
        auto tel = req.get_param_value("tel");
        auto paiement = req.get_param_value("paiement");

        std::string fichierPath;
        if (req.has_file("fichier")) {
            const auto& file = req.get_file_value("fichier");
            fichierPath = "uploads/" + file.filename;
            std::ofstream ofs(fichierPath, std::ios::binary);
            ofs << file.content;
        }

        insertAnnonce(titre, desc, tel, paiement, fichierPath);
        res.set_content("<h2>Annonce envoyée, en attente de validation.</h2>", "text/html");
    });

    // Page admin
    svr.Get("/admin", [](const httplib::Request&, httplib::Response& res) {
        res.set_content(getAnnoncesAdmin(), "text/html");
    });

    // Validation
    svr.Post("/valider", [](const httplib::Request& req, httplib::Response& res) {
        int id = std::stoi(req.get_param_value("id"));
        validerAnnonce(id);
        res.set_content("<h2>Annonce validée ✅</h2><a href='/admin'>Retour</a>", "text/html");
    });

    // Page publique
    svr.Get("/annonces", [](const httplib::Request&, httplib::Response& res) {
        res.set_content(getAnnoncesValidees(), "text/html");
    });

    // Servir les fichiers uploadés
    svr.set_mount_point("/uploads", "./uploads");

    std::cout << "Serveur STAF en écoute sur http://127.0.0.1:3000" << std::endl;
    svr.listen("127.0.0.1", 3000); 
    return 0;
}