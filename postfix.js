
var htsfiles = {};

function Htsfile(fileobj, progress_callback) {
    this.reader = {};
    this.offset = 0;
    this.cursor = -1;
    this.bufsize = 4194304; // 4 MiB
    this.buf = undefined;
    this.eof = 0;
    this.last_chunk = 0;
    this.fileobj = fileobj;
    this.progress_callback = progress_callback;
}

Htsfile.prototype._getchunk = function () {
    console.log('fetchfetch');
    var hts_fetch_js = cwrap('hts_fetch_js', 'number', ['number', 'string']);
    if (hts_fetch_js(0,"") == 0) {
        return fd;
    }
    if (this.fileobj.size > this.offset+this.bufsize) {
        this.last_chunk = 0;
        blob = this.fileobj.slice(this.offset, this.offset+this.bufsize);
    } else {
        blob = this.fileobj.slice(this.offset, this.fileobj.size);
        this.last_chunk = 1;
    }
    this.offset += this.bufsize;
    this.buf = hts_fetch_js; // todo    this.reader.readAsArrayBuffer(blob);
    this.cursor = 0;
    if (this.progress_callback)
        this.progress_callback(this.offset/this.fileobj.size*100);
}

Htsfile.prototype.seek = function (offset, whence) {
    var prev_offset = this.offset - this.bufsize;
    if (whence >= 0 && whence <= 2) {
        if (whence == 1)
            offset += prev_offset;
        if (whence == 2)
            offset = this.fileobj.size + offset;
        if (prev_offset <= offset && offset < this.offset) {
            this.cursor = offset - prev_offset;
        } else {
            this.offset = offset;
            this.cursor = -1;
            this.last_chunk = 0;
        }
        this.eof = 0;
        return offset;
    }
    return -1;
}

Htsfile.prototype.read = function (ptr, nbytes) {
    var buf, heap, over, nbytesread;

    if (this.last_chunk && this.cursor == -1) this.eof = 1;
    if (this.eof) return 0;

    nbytesread = 0;
    heap = new Int8Array(Module.HEAP8.buffer, ptr, nbytes);
    while (1) {
        if (this.cursor == -1) {
            if (this.last_chunk) {
                this.eof = 1;
                break;
            } else this._getchunk();
        }

        if (this.bufsize > this.cursor+nbytes) {
            // read part of buffer
            buf = this.buf.slice(this.cursor, this.cursor+nbytes);
            heap.set(new Int8Array(buf));
            nbytesread = buf.byteLength;

            this.cursor += nbytes;
        } else {
            // read from cursor to the end of buffer
            buf = this.buf.slice(this.cursor, this.bufsize);
            heap.set(new Int8Array(buf), nbytesread);
            nbytesread += buf.byteLength;

            this.cursor = -1;
        }
        nbytes -= nbytesread;
        if (nbytes < this.bufsize) break;
    }
    return nbytesread;
}

Htsfile.prototype.close = function () {
}

function js_close(fd) {
    htsfiles[fd].close();
    delete htsfiles[fd];
}

function js_seek(fd, offset, whence) {
    return htsfiles[fd].seek(offset, whence);
}

function js_read(fd, ptr, nbytes) {
    return htsfiles[fd].read(ptr, nbytes);
}

function hts_open(filename, progress_callback) {
    htsfiles[filename] = new Htsfile(filename, progress_callback);;
    console.log('herehrehre');

    var hts_open_js = cwrap('hts_open_js', 'number', ['string']);
    if (hts_open_js(filename, fileobj.name) == 0)
        return fd;
    else
        throw "Something wrong happened while opening file.";
        return 0;
}

function hts_close(fd) {
    Module._hts_close_js(fd);
}

function run_pileup(fd_bam, fd_bai, fd_fa, fd_fai, reg) {
    var func = cwrap('run_pileup', 'number', ['string','string', 'string']);
    func(fd_bam, fd_bai, fd_fa, fd_fai, reg);
}

self["hts_open"] = hts_open;
self["hts_close"] = hts_close;
self["run_pileup"] = run_pileup;
