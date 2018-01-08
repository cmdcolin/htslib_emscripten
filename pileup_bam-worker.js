importScripts('pileup_bam.js');

progress_callback = function (p) {
    postMessage([1, p]);
};

onmessage = function(e) {
    t1 = performance.now();
    cmd = e.data[0];
    if (cmd === 1) {

    }
    t2 = performance.now();
    postMessage([3, (t2 - t1)/1000]);
};
