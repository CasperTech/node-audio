const { AudioPlayer } = require('node-cmake')('node_audio')

async function run()
{
    var obj = new AudioPlayer();
    console.log('Loading..');
    //await obj.load('F:\\Music\\Freestylers - Adventures In Freestyle\\3 Could I Be Dreaming-.flac');
    console.log('Done');
}
run().then(() => {
    console.log('Done all!');
}).catch((err) => {
    console.error(err);
})

