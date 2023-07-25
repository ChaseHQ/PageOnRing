// PageOnRing.c
//

#include <pjsua-lib/pjsua.h>
#include <SDL3/SDL_mixer.h>
#include <SDL3/SDL.h>
#include <signal.h>
#include <string.h>

#define THIS_FILE "PageOnRing.c"

char* _SIP_DOMAIN;
char* _SIP_USER;
char* _SIP_PASSWD;
char* _SIP_REALM;
char* _SIP_ID;
char* _SIP_REGID;
char* _AUDIO_DEVICE;
char* _AUDIO_FILE;
int   _AUDIO_LOOP_NUM = -1;

Mix_Music* _pageSoundfile;
int _shutdown = 0;

/* Callback called by the library upon receiving incoming call */
static void on_incoming_call(pjsua_acc_id acc_id, pjsua_call_id call_id,
 pjsip_rx_data *rdata)
{
	pjsua_call_info ci;

	PJ_UNUSED_ARG(acc_id);
	PJ_UNUSED_ARG(rdata);

	pjsua_call_get_info(call_id, &ci);

	PJ_LOG(3,(THIS_FILE, "Incoming call from %.*s!!",
	(int)ci.remote_info.slen,
	ci.remote_info.ptr));

	PJ_LOG(3, (THIS_FILE, "Playing Audio File - '%s'", _AUDIO_FILE));
	Mix_PlayMusic(_pageSoundfile, _AUDIO_LOOP_NUM);
}

/* Callback called by the library when call's state has changed */
static void on_call_state(pjsua_call_id call_id, pjsip_event *e)
{
	pjsua_call_info ci;

	PJ_UNUSED_ARG(e);

	pjsua_call_get_info(call_id, &ci);
	PJ_LOG(3,(THIS_FILE, "Call %d state=%.*s", call_id,
	(int)ci.state_text.slen,
	ci.state_text.ptr));
 
	if (ci.state == PJSIP_INV_STATE_DISCONNECTED) {
		if (Mix_PlayingMusic()) {
			PJ_LOG(3, (THIS_FILE, "Audio: Ending Paging Music"));
			Mix_FadeOutMusic(500);
		}
	}
}

/* Display error and exit application */
static void error_exit(const char *title, pj_status_t status)
{
	pjsua_perror(THIS_FILE, title, status);
	pjsua_destroy();
	exit(1);
 }

void listAudioDevices() {
	SDL_Init(SDL_INIT_AUDIO);

	int d;
	for (d = 0; d < SDL_GetNumAudioDevices(0); ++d) {
		PJ_LOG(3, (THIS_FILE, "AudioDeviceFound: %s", SDL_GetAudioDeviceName(d, 0)));
	}
}
 
int setupSDL() {

	if (Mix_OpenAudioDevice(44100, MIX_DEFAULT_FORMAT, MIX_DEFAULT_CHANNELS, 2048, _AUDIO_DEVICE, SDL_AUDIO_ALLOW_ANY_CHANGE)) {
		PJ_LOG(1, (THIS_FILE, "MixError: %s", Mix_GetError()));
		return 1;
	}
	
	if ((MIX_INIT_MP3) != Mix_Init(MIX_INIT_MP3)) {
		PJ_LOG(1, (THIS_FILE, "MixError: %s", Mix_GetError()));
		return 1;
	}

	_pageSoundfile = Mix_LoadMUS(_AUDIO_FILE);
	
	if (_pageSoundfile == NULL) {
		PJ_LOG(1, (THIS_FILE, "MixError: %s", Mix_GetError()));
		return 1;
	}

	return 0;
}

void signal_handler(int s) {
	if (s == SIGINT || s == SIGTERM) {
		_shutdown = 1;
	}
}

void createGlobalArgument(char** GLOBAL, const char* arg) {
	*GLOBAL = (char*)malloc(strlen(arg) + 1);
	memset(*GLOBAL, 0, strlen(arg) + 1);
	strcpy(*GLOBAL, arg);
}

void setupGlobalArguments(char* argv[]) {
	createGlobalArgument(&_SIP_DOMAIN, argv[1]);
	createGlobalArgument(&_SIP_USER, argv[2]);
	createGlobalArgument(&_SIP_PASSWD, argv[3]);
	createGlobalArgument(&_SIP_REALM, argv[4]);
	createGlobalArgument(&_AUDIO_DEVICE, argv[5]);
	createGlobalArgument(&_AUDIO_FILE, argv[6]);
	_AUDIO_LOOP_NUM = atoi(argv[7]);

	_SIP_ID = (char*)malloc(strlen("sip:") + strlen(_SIP_USER) + strlen("@") + strlen(_SIP_DOMAIN) + 1);
	memset(_SIP_ID, 0, strlen("sip:") + strlen(_SIP_USER) + strlen("@") + strlen(_SIP_DOMAIN) + 1);
	strcat(_SIP_ID, "sip:");
	strcat(_SIP_ID, _SIP_USER);
	strcat(_SIP_ID, "@");
	strcat(_SIP_ID, _SIP_DOMAIN);

	_SIP_REGID = (char*)malloc(strlen("sip:") + strlen(_SIP_DOMAIN) + 1);
	memset(_SIP_REGID, 0, strlen("sip:") + strlen(_SIP_DOMAIN) + 1);
	strcat(_SIP_REGID, "sip:");
	strcat(_SIP_REGID, _SIP_DOMAIN);
}

void destroyGlobalArguments() {
	free(_SIP_DOMAIN);
	free(_SIP_PASSWD);
	free(_SIP_REALM);
	free(_SIP_USER);
	free(_SIP_ID);
	free(_SIP_REGID);
	free(_AUDIO_DEVICE);
	free(_AUDIO_FILE);
}

 int main(int argc, char *argv[])
 {

	 puts("PageOnRing v1.0 - Register a SIP Extension and Send Audio out device while ringing.");
	 puts("(c) 2023 - Craig Vella\n");
	 
	 pjsua_acc_id acc_id;
	 pj_status_t status;
 	 
	 /* Create pjsua first! */
	 status = pjsua_create();
	 if (status != PJ_SUCCESS) error_exit("Error in pjsua_create()", status);
 	 
	 /* Init pjsua */
	 {
	 	pjsua_config cfg;
	 	pjsua_logging_config log_cfg;
 	 
	 	pjsua_config_default(&cfg);
	 	cfg.cb.on_incoming_call = &on_incoming_call;
	 	cfg.cb.on_call_state = &on_call_state;
 	 
	 	pjsua_logging_config_default(&log_cfg);
	 	log_cfg.console_level = 3;
 	 
	 	status = pjsua_init(&cfg, &log_cfg, NULL);
	 	if (status != PJ_SUCCESS) error_exit("Error in pjsua_init()", status);
	 }

	 listAudioDevices();

	 if (argc < 8) {
		 PJ_LOG(1,(THIS_FILE,"Usage: PageOnRing <Domain> <Extension> <Password> <Realm> <\"AudioDeviceName\"> <\"MP3 to Play\"> <Num of Loops>"));
		 exit(1);
	 }

	 puts("");

	 setupGlobalArguments(argv);

	 signal(SIGINT, signal_handler);
	 signal(SIGTERM, signal_handler);

	 if (setupSDL()) {
		 error_exit("Error in setupSDL()", 1);
	 }

	 /* Add UDP transport. */
	 {
	 	pjsua_transport_config cfg;
 	 
	 	pjsua_transport_config_default(&cfg);
	 	cfg.port = 0;
	 	status = pjsua_transport_create(PJSIP_TRANSPORT_UDP, &cfg, NULL);
	 	if (status != PJ_SUCCESS) error_exit("Error creating transport", status);
	 }
 	 
	 /* Initialization is done, now start pjsua */
	 status = pjsua_start();
	 if (status != PJ_SUCCESS) error_exit("Error starting pjsua", status);
 	 
	 /* Register to SIP server by creating SIP account. */
	 {
	 	pjsua_acc_config cfg;
 	 
	 	pjsua_acc_config_default(&cfg);
	 	cfg.id = pj_str(_SIP_ID);
	 	cfg.reg_uri = pj_str(_SIP_REGID);
	 	cfg.cred_count = 1;
	 	cfg.cred_info[0].realm = pj_str(_SIP_REALM);
	 	cfg.cred_info[0].scheme = pj_str("digest");
	 	cfg.cred_info[0].username = pj_str(_SIP_USER);
	 	cfg.cred_info[0].data_type = PJSIP_CRED_DATA_PLAIN_PASSWD;
	 	cfg.cred_info[0].data = pj_str(_SIP_PASSWD);
 	 
	 	status = pjsua_acc_add(&cfg, PJ_TRUE, &acc_id);
	 	if (status != PJ_SUCCESS) error_exit("Error adding account", status);
	 }
 	 
	 /* Wait until shutdown */
	 while (!_shutdown) {
		 Sleep(10);
	 }

	 PJ_LOG(3, (THIS_FILE, "Shutting Down..."));
 	 
	 /* Destroy pjsua */
	 pjsua_destroy();
 	 
	 Mix_CloseAudio(); // Shutdown Sound 

	 destroyGlobalArguments();
	 
  return 0;
 }