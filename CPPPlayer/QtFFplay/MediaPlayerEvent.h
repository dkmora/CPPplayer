#pragma once

// 播放状态码
typedef enum MediaPlayerState {
	PLAYER_STATE_IDLE = 0,                     // 0  默认状态。播放器会在你打开媒体文件之前和结束播放之后返回该状态码。
	PLAYER_STATE_OPENING,                      // 1: 正在打开媒体文件
	PLAYER_STATE_OPEN_COMPLETED,               // 2: 成功打开媒体文件
	PLAYER_STATE_PLAYING,                      // 3: 正在播放
	PLAYER_STATE_PAUSED,                       // 4: 暂停播放
	PLAYER_STATE_PLAYBACK_COMPLETED,           // 5: 播放完毕
	PLAYER_STATE_PLAYBACK_ALL_LOOPS_COMPLETED, // 6: 循环播放已结束
	PLAYER_STATE_PLAYBACK_END_COMPLETED,       // 7: 播放已结束
	PLAYER_STATE_FAILED                        // 100: 播放失败
}MediaPlayerState;

// 播放错误码
typedef enum MediaPlayerError {
	PLAYER_ERROR_NONE = (0),                         // 0: 没有错误
	PLAYER_ERROR_INVALID_ARGUMENTS = (-1),           // -1: 不正确的参数
	PLAYER_ERROR_INTERNAL = (-2),                    // -2: 内部错误
	PLAYER_ERROR_NO_RESOURCE = (-3),                 // -3: 没有 resource
	PLAYER_ERROR_INVALID_MEDIA_SOURCE = (-4),        // -4: 无效的 resource
	PLAYER_ERROR_UNKNOWN_STREAM_TYPE = (-5),         // -5: 未知的媒体流类型
	PLAYER_ERROR_OBJ_NOT_INITIALIZED = (-6),         // -6: 对象没有初始化
	PLAYER_ERROR_CODEC_NOT_SUPPORTED = (-7),         // -7: 解码器不支持该 codec
	PLAYER_ERROR_VIDEO_RENDER_FAILED = (-8),         // -8: 无效的 renderer
	PLAYER_ERROR_INVALID_STATE = (-9),               // -9: 播放器内部状态错误
	PLAYER_ERROR_URL_NOT_FOUND = (-10),              // -10: 未找到该 URL
	PLAYER_ERROR_INVALID_CONNECTION_STATE = (-11),   // -11: 播放器与 Agora 服务器的连接无效
	PLAY_ERROR_SRC_BUFFER_UNDERFLOW = (-12),         // -12: 播放缓冲区数据不足
	PLAYER_ERROR_READFRAME_TIMEOUT = (-13)           // -13: av_read_frame timeout
}MediaPlayerError;

class MediaPlayerEventHandler {
public:
	virtual ~MediaPlayerEventHandler() {}

	virtual void onPlayerStateChange(MediaPlayerState state, MediaPlayerError error) {

	}
};