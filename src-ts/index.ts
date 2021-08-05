import {PlaybackEvent} from "./PlaybackEvent";

const audioPlayer = require('node-cmake')('node_audio')

export class AudioPlayer
{
    private player;

    constructor()
    {
        this.player = new audioPlayer.AudioPlayer();
    }

    public load(fileName: string): Promise<void>
    {
        return this.player.load(fileName);
    }

    public play(): Promise<void>
    {
        return this.player.play();
    }

    public pause(): Promise<void>
    {
        return this.player.pause();
    }

    public seek(ms: number): Promise<void>
    {
        return this.player.seek(ms);
    }

    public stop(): Promise<void>
    {
        return this.player.stop();
    }

    public setEventCallback(cb: (event: PlaybackEvent, msg: string) => void)
    {
        this.player.setEventCallback(cb);
    }
}