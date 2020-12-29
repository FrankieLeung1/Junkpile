#include "stdafx.h"
#include "WebServer.h"
#include "../Misc/Misc.h"
#include "../imgui/ImGuiManager.h"

#pragma warning (push)
#pragma warning( disable : 4267 )
#include "EmbeddableWebServer/EmbeddableWebServer.h"
#pragma warning ( pop )

#ifdef WIN32
#pragma comment(lib, "ws2_32") // link against Winsock on Windows
#endif

namespace {
    extern const char* defaultHTML; // assigned below
}

WebServer::WebServer():
m_thread(nullptr),
m_port(80),
m_body(defaultHTML)
{
}

WebServer::~WebServer()
{
	disable();
}

void WebServer::test()
{
	WebServer* server = createTestResource<WebServer>();
	ResourcePtr<ImGuiManager> m;
	m->registerCallback({ [](void* ud) {
		WebServer* server = static_cast<WebServer*>(ud);

		ImGui::Begin("WebServer");
		ImGui::InputTextMultiline("HTML", &server->m_body, ImVec2(-1, -20.0f));
		bool started = (server->m_thread != nullptr);
		if (ImGui::Button(!started ? "Start" : "Stop"))
		{
			if (started) server->disable();
			else server->enable();
		}
		ImGui::SameLine();

		if (started)
		{
			if (ImGui::Button("Visit"))
				ShellExecute(NULL, L"open", L"http://localhost/", NULL, NULL, SW_SHOWNORMAL);

			ImGui::SameLine();
		}
		ImGui::InputInt("port", &server->m_port, 1, 100, started ? ImGuiInputTextFlags_ReadOnly : 0);
		ImGui::End();

	}, server });
}

void WebServer::enable()
{
	if (m_thread && m_server)
		return;

	m_thread = std::make_shared<std::thread>(&WebServer::entry, this, m_port);

	m_server = new Server{ 0 };
	serverInit(m_server);
	m_server->tag = this;
}

void WebServer::disable()
{
	if (!m_thread)
		return;

	if (m_server)
	{
		serverStop(m_server);
		serverDeInit(m_server);
		m_server = nullptr;
	}
	// BUG: joining here seems to block forever
	m_thread->detach();
	m_thread = nullptr;
}

struct Response* createResponseForRequest(const struct Request* request, struct Connection* connection)
{
	WebServer* instance = static_cast<WebServer*>(connection->server->tag);
	std::lock_guard<std::mutex> l(instance->m_mutex);
    return responseAllocHTML(instance->m_body.c_str());
	//return responseAllocWithFormat(200, "OK", "text/html; charset=UTF-8", instance->m_body.c_str());
}

void WebServer::entry(WebServer* instance, int port)
{
	acceptConnectionsUntilStoppedFromEverywhereIPv4(instance->m_server, port);
}

namespace {
    const char* defaultHTML = R"delim(<!DOCTYPE html>
<html>
<head>
<meta name = "viewport" content = "width=device-width, initial-scale=1.0" / >
<style>
canvas{
    border:1px solid #d3d3d3;
    background - color: #f1f1f1;
}
</style>
</head>
<body onload = "startGame()">
<script>

var myGamePiece;
var myObstacles = [];
var myScore;

function startGame() {
    myGamePiece = new component(30, 30, "red", 10, 120);
    myGamePiece.gravity = 0.05;
    myScore = new component("30px", "Consolas", "black", 280, 40, "text");
    myGameArea.start();
}

var myGameArea = {
    canvas: document.createElement("canvas"),
    start : function() {
        this.canvas.width = 480;
        this.canvas.height = 270;
        this.context = this.canvas.getContext("2d");
        document.body.insertBefore(this.canvas, document.body.childNodes[0]);
        this.frameNo = 0;
        this.interval = setInterval(updateGameArea, 20);
        },
    clear : function() {
        this.context.clearRect(0, 0, this.canvas.width, this.canvas.height);
    }
}

function component(width, height, color, x, y, type) {
    this.type = type;
    this.score = 0;
    this.width = width;
    this.height = height;
    this.speedX = 0;
    this.speedY = 0;
    this.x = x;
    this.y = y;
    this.gravity = 0;
    this.gravitySpeed = 0;
    this.update = function() {
        ctx = myGameArea.context;
        if (this.type == "text") {
            ctx.font = this.width + " " + this.height;
            ctx.fillStyle = color;
            ctx.fillText(this.text, this.x, this.y);
        }
        else {
            ctx.fillStyle = color;
            ctx.fillRect(this.x, this.y, this.width, this.height);
        }
    }
    this.newPos = function() {
        this.gravitySpeed += this.gravity;
        this.x += this.speedX;
        this.y += this.speedY + this.gravitySpeed;
        this.hitBottom();
    }
    this.hitBottom = function() {
        var rockbottom = myGameArea.canvas.height - this.height;
        if (this.y > rockbottom) {
            this.y = rockbottom;
            this.gravitySpeed = 0;
        }
    }
    this.crashWith = function(otherobj) {
        var myleft = this.x;
        var myright = this.x + (this.width);
        var mytop = this.y;
        var mybottom = this.y + (this.height);
        var otherleft = otherobj.x;
        var otherright = otherobj.x + (otherobj.width);
        var othertop = otherobj.y;
        var otherbottom = otherobj.y + (otherobj.height);
        var crash = true;
        if ((mybottom < othertop) || (mytop > otherbottom) || (myright < otherleft) || (myleft > otherright)) {
            crash = false;
        }
        return crash;
    }
}

function updateGameArea() {
    var x, height, gap, minHeight, maxHeight, minGap, maxGap;
    for (i = 0; i < myObstacles.length; i += 1) {
        if (myGamePiece.crashWith(myObstacles[i])) {
            return;
        }
    }
    myGameArea.clear();
    myGameArea.frameNo += 1;
    if (myGameArea.frameNo == 1 || everyinterval(150)) {
        x = myGameArea.canvas.width;
        minHeight = 20;
        maxHeight = 200;
        height = Math.floor(Math.random() * (maxHeight - minHeight + 1) + minHeight);
        minGap = 50;
        maxGap = 200;
        gap = Math.floor(Math.random() * (maxGap - minGap + 1) + minGap);
        myObstacles.push(new component(10, height, "green", x, 0));
        myObstacles.push(new component(10, x - height - gap, "green", x, height + gap));
    }
    for (i = 0; i < myObstacles.length; i += 1) {
        myObstacles[i].x += -1;
        myObstacles[i].update();
    }
    myScore.text = "SCORE: " + myGameArea.frameNo;
    myScore.update();
    myGamePiece.newPos();
    myGamePiece.update();
}

function everyinterval(n) {
    if ((myGameArea.frameNo / n) % 1 == 0) { return true; }
    return false;
}

function accelerate(n) {
    myGamePiece.gravity = n;
}
</script>
<br>
<button onmousedown = "accelerate(-0.2)" onmouseup = "accelerate(0.05)">ACCELERATE</button>
<p>Use the ACCELERATE button to stay in the air</p>
<p>How long can you stay alive ? </p>
</body>
</html>
)delim";
}