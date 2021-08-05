import { PlaybackEvent } from "./PlaybackEvent";
export declare class AudioPlayer {
    private player;
    constructor();
    load(fileName: string): Promise<void>;
    play(): Promise<void>;
    pause(): Promise<void>;
    seek(ms: number): Promise<void>;
    stop(): Promise<void>;
    setEventCallback(cb: (event: PlaybackEvent, msg: string) => void): void;
}
