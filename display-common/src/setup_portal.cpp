#include "setup_portal.h"
#include "dd_config.h"
#include "gfx.h"

#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>

namespace {
    WebServer server(80);

    // The schema for the running portal, captured by run() so the plain WebServer
    // handler functions can reach it.
    setup_portal::Field*    g_fields = nullptr;
    size_t                  g_n      = 0;
    setup_portal::SaveFn    g_save;
    const char*             g_title  = "Setup";
    const char*             g_intro  = "";

    // Shared Swiss-design chrome (matches the on-screen Helvetica / minimal look).
    const char* CSS =
        "body{font-family:'Helvetica Neue',Helvetica,Arial,sans-serif;max-width:540px;"
        "margin:1.5em auto;padding:0 1em;color:#222;line-height:1.4}"
        "h1{font-size:1.5em;margin-bottom:0.3em}"
        ".lede{color:#555;margin-top:0;margin-bottom:1em}"
        ".banner{background:#fff3cd;border:1px solid #ffeaa7;padding:0.8em 1em;"
        "border-radius:4px;margin:1em 0;font-size:0.92em}"
        ".banner a{color:#0066cc}"
        "label{display:block;margin-top:1.1em;font-weight:600}"
        "input{width:100%;padding:0.55em;font-size:1em;border:1px solid #ccc;"
        "border-radius:4px;font-family:inherit;box-sizing:border-box}"
        "input:focus{outline:none;border-color:#0066cc;box-shadow:0 0 0 2px rgba(0,102,204,0.15)}"
        ".hint{font-size:0.85em;color:#666;margin-top:0.3em}"
        ".hint a{color:#0066cc}"
        "button{margin-top:1.6em;padding:0.7em 1.6em;background:#0066cc;color:white;"
        "border:none;border-radius:4px;font-size:1em;font-family:inherit;cursor:pointer}"
        "button:hover{background:#0055aa}"
        ".opt{font-weight:normal;color:#666;font-size:0.9em}"
        "a{color:#0066cc}";

    String esc(const String& in) {
        String s;
        s.reserve(in.length());
        for (size_t i = 0; i < in.length(); ++i) {
            char c = in.charAt(i);
            switch (c) {
                case '&': s += "&amp;";  break;
                case '<': s += "&lt;";   break;
                case '>': s += "&gt;";   break;
                case '"': s += "&quot;"; break;
                default:  s += c;        break;
            }
        }
        return s;
    }

    String page_head(const String& title) {
        String h = F("<!DOCTYPE html><html lang=\"en\"><head><meta charset=\"utf-8\">"
                     "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\"><title>");
        h += esc(title);
        h += F("</title><style>");
        h += CSS;
        h += F("</style></head><body>");
        return h;
    }

    String render_form() {
        String html = page_head(g_title);
        html += "<h1>"; html += esc(g_title); html += "</h1>";
        if (g_intro && g_intro[0]) {
            html += "<div class=\"banner\">"; html += g_intro; html += "</div>";
        }
        html += F("<form method=\"post\" action=\"/save\">");

        for (size_t i = 0; i < g_n; ++i) {
            const setup_portal::Field& f = g_fields[i];
            html += "<label>";
            html += esc(f.label);
            if (f.optional) html += " <span class=\"opt\">(optional)</span>";
            html += "</label>";

            const char* type = f.type == setup_portal::NUMBER   ? "number"
                             : f.type == setup_portal::PASSWORD ? "password"
                                                                : "text";
            html += "<input name=\"";
            html += f.key;
            html += "\" type=\"";
            html += type;
            html += "\" value=\"";
            html += esc(f.value);
            html += "\"";
            if (f.placeholder) { html += " placeholder=\""; html += esc(f.placeholder); html += "\""; }
            if (!f.optional) html += " required";
            if (f.type == setup_portal::NUMBER && f.max > f.min) {
                html += " min=\""; html += String(f.min); html += "\"";
                html += " max=\""; html += String(f.max); html += "\"";
            }
            if (i == 0) html += " autofocus";
            html += " autocomplete=\"off\">";

            if (f.hint) { html += "<div class=\"hint\">"; html += f.hint; html += "</div>"; }
        }

        html += F("<button type=\"submit\">Save</button></form></body></html>");
        return html;
    }

    String error_page(const String& msg) {
        String html = page_head(g_title);
        html += "<h1>Check the form</h1><p>";
        html += esc(msg);
        html += "</p><p><a href=\"/setup\">Back to form</a></p></body></html>";
        return html;
    }

    String saved_page() {
        String html = page_head("Saved");
        html += F("<h1>Saved.</h1><p>The device is rebooting and will start up in a moment.</p></body></html>");
        return html;
    }

    void serve_form() { server.send(200, "text/html", render_form()); }

    void handle_save() {
        std::map<String,String> values;
        String err;

        for (size_t i = 0; i < g_n; ++i) {
            const setup_portal::Field& f = g_fields[i];
            String v = server.arg(f.key);
            v.trim();

            if (!f.optional && v.length() == 0) {
                err = String("\"") + f.label + "\" is required.";
                server.send(400, "text/html", error_page(err));
                return;
            }
            if (f.type == setup_portal::NUMBER && v.length() > 0 && f.max > f.min) {
                long n = v.toInt();
                if (n < f.min || n > f.max) {
                    err = String("\"") + f.label + "\" must be between "
                          + f.min + " and " + f.max + ".";
                    server.send(400, "text/html", error_page(err));
                    return;
                }
            }
            // Keep the submitted value so a re-render after an app-side error keeps input.
            g_fields[i].value = v;
            values[f.key] = v;
        }

        if (!g_save || !g_save(values, err)) {
            server.send(400, "text/html", error_page(err.length() ? err : String("Could not save.")));
            return;
        }

        server.send(200, "text/html", saved_page());
        delay(1500);
        ESP.restart();
    }

    void redirect_to_setup() {
        server.sendHeader("Location", "/setup");
        server.send(302, "text/plain", "");
    }
}

namespace setup_portal {

[[noreturn]] void run(const char* mdns_host, const char* title,
                      const char* intro_html,
                      Field* fields, size_t n, SaveFn on_save) {
    g_fields = fields;
    g_n      = n;
    g_save   = on_save;
    g_title  = title;
    g_intro  = intro_html ? intro_html : "";

    MDNS.begin(mdns_host);
    MDNS.addService("http", "tcp", 80);

    server.on("/",      HTTP_GET,  serve_form);
    server.on("/setup", HTTP_GET,  serve_form);
    server.on("/save",  HTTP_POST, handle_save);
    server.onNotFound(redirect_to_setup);
    server.begin();

    gfx::show_setup_url(WiFi.localIP(), mdns_host);

    while (true) {
        server.handleClient();
        delay(10);
    }
}

} // namespace setup_portal
