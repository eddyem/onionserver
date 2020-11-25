auth = function(){
	var wsKey = "";
	function _ilogin(){
		$("inout").innerHTML = "Log in";
		$("inout").onclick = auth.login;
	}
	function _ilogout(){
		$("shadow").style.display = "none";
		$("inout").innerHTML = "Log out";
		$("inout").onclick = auth.logout;
	}
	function _wsk(request){
		wsKey = request.responseText;
		if(wsKey) console.log("Web key received: " + wsKey);
		wsinit();
	}
	function reqAuth(request){
		var txt = request.responseText;
		dbg("received " + txt);
		if(txt == "AuthOK"){ // cookies received
			sendrequest("get/?getWSkey", _wsk);
			_ilogout();
		}else if(txt == "NeedAuth"){
			_ilogin();
		}else{
			parseErr(txt);
		}
	}
	function init1(){
		sendrequest("auth/?check=1", reqAuth);
		var l = document.createElement('a');
		l.id = "inout";
		l.href = "#";
		var s1 = document.createElement('style');
		s1.type = 'text/css';
		s1.innerHTML = ".inout{position:absolute;top:0;right:0;background-color:green;"
		document.body.appendChild(s1);
		l.className = "inout";
		document.body.appendChild(l);
		var d = document.createElement('div');
		d.id = "shadow";
		var s = document.createElement('style');
		s.type = 'text/css';
		s.innerHTML = '.shadow{position:absolute;text-align:center;vertical-align:center;top:0;display:none;left:0;width:100%;height:100%;background-color:lightGrey;opacity:0.9;}';
		document.body.appendChild(s);
		d.innerHTML = 	"<form autocomplete='on'><div>Login:</div><div><input type='text' id='login' name='username'>" +
						"</div><div>Password:</div><div><input type='password' id='passwd' name='password'>" +
						"</div><button onclick='auth.send();'>OK</button></form>";
		d.className = "shadow";
		document.body.appendChild(d);
	}
	function login1(){
		$("shadow").style.display = "block";
	}
	function logout1(){
		sendrequest("auth/?LogOut=1", _ilogin);
		ws.close();
	}
	function sendlogpass(){
		$("shadow").style.display = "none";
		var l = $("login").value, p = $("passwd").value;
		if(!l || !p){
			parseErr("give login and password");
			return;
		}
		var str = "auth/?login=" + l + "&passwd=" + p;
		sendrequest(str, reqAuth);
	}
	// websockets
	var ws;
	var reopenTimer;
	function wsinit(){
		delete(ws);
		console.log("Try to connect WS");
		ws = new WebSocket('wss://localhost:8080/ws/');
		clearTimeout(reopenTimer);
		reopenTimer = setTimeout(wsinit, 5000);
		ws.onopen = function(){ // send key after init
			clearTimeout(reopenTimer);
			console.log("WS connected");
			ws.send("Akey="+wsKey);
		}
		ws.onclose = function(evt){
			var text = "websocket closed";
			if(evt.wasClean) text += " by remote side";
			else text += ", connection lost"
			parseErr(text);
			console.log("WS closed");
			clearTimeout(reopenTimer);
			reopenTimer = setTimeout(wsinit, 1000);
		}
		ws.onmessage = function(evt){
			$('wsmsgs').innerHTML = evt.data;
		}
		ws.onerror = function(err){
			parseErr("WebSocket error " + err.message);
		}
	}
	function wssend(txt){
		ws.send(txt);
	}
	return{
		init: init1,
		login: login1,
		logout: logout1,
		send: sendlogpass,
		wssend: wssend,
		wsinit: wsinit
	};
}();
