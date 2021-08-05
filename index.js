"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.AudioPlayer = void 0;
const audioPlayer = require('node-cmake')('node_audio');
class AudioPlayer {
    constructor() {
        this.player = new audioPlayer.AudioPlayer();
    }
    load(fileName) {
        return this.player.load(fileName);
    }
    play() {
        return this.player.play();
    }
    pause() {
        return this.player.pause();
    }
    seek(ms) {
        return this.player.seek(ms);
    }
    stop() {
        return this.player.stop();
    }
    setEventCallback(cb) {
        this.player.setEventCallback(cb);
    }
}
exports.AudioPlayer = AudioPlayer;
//# sourceMappingURL=index.js.map