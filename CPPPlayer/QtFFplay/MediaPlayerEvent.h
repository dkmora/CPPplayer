#pragma once

// ����״̬��
typedef enum MediaPlayerState {
	PLAYER_STATE_IDLE = 0,                     // 0  Ĭ��״̬���������������ý���ļ�֮ǰ�ͽ�������֮�󷵻ظ�״̬�롣
	PLAYER_STATE_OPENING,                      // 1: ���ڴ�ý���ļ�
	PLAYER_STATE_OPEN_COMPLETED,               // 2: �ɹ���ý���ļ�
	PLAYER_STATE_PLAYING,                      // 3: ���ڲ���
	PLAYER_STATE_PAUSED,                       // 4: ��ͣ����
	PLAYER_STATE_PLAYBACK_COMPLETED,           // 5: �������
	PLAYER_STATE_PLAYBACK_ALL_LOOPS_COMPLETED, // 6: ѭ�������ѽ���
	PLAYER_STATE_PLAYBACK_END_COMPLETED,       // 7: �����ѽ���
	PLAYER_STATE_FAILED                        // 100: ����ʧ��
}MediaPlayerState;

// ���Ŵ�����
typedef enum MediaPlayerError {
	PLAYER_ERROR_NONE = (0),                         // 0: û�д���
	PLAYER_ERROR_INVALID_ARGUMENTS = (-1),           // -1: ����ȷ�Ĳ���
	PLAYER_ERROR_INTERNAL = (-2),                    // -2: �ڲ�����
	PLAYER_ERROR_NO_RESOURCE = (-3),                 // -3: û�� resource
	PLAYER_ERROR_INVALID_MEDIA_SOURCE = (-4),        // -4: ��Ч�� resource
	PLAYER_ERROR_UNKNOWN_STREAM_TYPE = (-5),         // -5: δ֪��ý��������
	PLAYER_ERROR_OBJ_NOT_INITIALIZED = (-6),         // -6: ����û�г�ʼ��
	PLAYER_ERROR_CODEC_NOT_SUPPORTED = (-7),         // -7: ��������֧�ָ� codec
	PLAYER_ERROR_VIDEO_RENDER_FAILED = (-8),         // -8: ��Ч�� renderer
	PLAYER_ERROR_INVALID_STATE = (-9),               // -9: �������ڲ�״̬����
	PLAYER_ERROR_URL_NOT_FOUND = (-10),              // -10: δ�ҵ��� URL
	PLAYER_ERROR_INVALID_CONNECTION_STATE = (-11),   // -11: �������� Agora ��������������Ч
	PLAY_ERROR_SRC_BUFFER_UNDERFLOW = (-12),         // -12: ���Ż��������ݲ���
	PLAYER_ERROR_READFRAME_TIMEOUT = (-13)           // -13: av_read_frame timeout
}MediaPlayerError;

class MediaPlayerEventHandler {
public:
	virtual ~MediaPlayerEventHandler() {}

	virtual void onPlayerStateChange(MediaPlayerState state, MediaPlayerError error) {

	}
};