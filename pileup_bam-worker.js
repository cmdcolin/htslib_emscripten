importScripts('pileup_bam.js');

progress_callback = function (p) {
    postMessage([1, p]);
};

onmessage = function(e) {
    t1 = performance.now();
    cmd = e.data[0];
    if (cmd === 1) {
        fd_bam = hts_open(e.data[1], progress_callback);
        fd_bai = hts_open(e.data[2]);
        run_pileup(fd_bam, fd_bai, e.data[3]);
        hts_close(fd_bam); 
        hts_close(fd_bai); 
    }
    t2 = performance.now();
    postMessage([3, (t2 - t1)/1000]);
};

