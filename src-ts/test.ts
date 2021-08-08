import { AudioPlayer } from './index';
import * as path from 'path';

const sleep = async(ms: number): Promise<void> =>
{
    return new Promise<void>((resolve) =>
    {
        setTimeout(() =>
        {
            resolve();
        }, ms)
    });
};

(async () =>
{
    const testPlayer = new AudioPlayer();
    console.log('Loading file..')

    const mediaPath = path.join(__dirname, 'media');
    const sampleFile = path.join(mediaPath, 'piano.wav');
    await testPlayer.load(sampleFile);

    console.log('File loaded. Seeking to 10 seconds');
    await testPlayer.seek(2000);

    console.log('Waiting 2 seconds before playback..');
    await sleep(2000);

    console.log('Playing for 5 seconds');
    await testPlayer.play();

    console.log('Waiting for 5 seconds');
    await sleep(5000);

    console.log('Seeking back to 0');
    await testPlayer.seek(0);
    console.log('Waiting for 5 seconds then seeking back to 0');
    await sleep(100);
    await testPlayer.seek(0);
    await sleep(25000);

})().catch((err) =>
{
    console.error(err);
});

