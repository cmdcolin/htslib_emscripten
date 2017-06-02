define([
    'dojo/_base/declare',
    'dojo/_base/array',
    'JBrowse/has',
    'JBrowse/Util',
    'JBrowse/Errors',
    'JBrowse/Store/LRUCache',
    './Util',
    './LazyFeature',
    './Htslib'
],
function (
    declare,
    array,
    has,
    Util,
    Errors,
    LRUCache,
    CRAMUtil,
    CRAMFeature,
    Htslib
) {
    var CRAM_MAGIC = 1129464141;
    var CRAI_MAGIC = 21578050;

    var dlog = function () { console.error.apply(console, arguments); };

    var Chunk = Util.fastDeclare({
        constructor: function (minv, maxv, bin) {
            this.minv = minv;
            this.maxv = maxv;
            this.bin = bin;
        },
        toUniqueString: function () {
            return this.minv + '..' + this.maxv + ' (bin ' + this.bin + ')';
        },
        toString: function () {
            return this.toUniqueString();
        },
        fetchedSize: function () {
            return this.maxv.block + (1 << 16) - this.minv.block + 1;
        }
    });

    var readInt   = CRAMUtil.readInt;
    var readVirtualOffset = CRAMUtil.readVirtualOffset;

    return declare(null, {
        constructor: function (args) {
            this.store = args.store;
            this.data  = args.data;
            this.crai   = args.crai;

            // initialize module
            this.Module = {};
            Htslib(this.Module);

            this.chunkSizeLimit = args.chunkSizeLimit || 5000000;
        },

        init: function (args) {
            var cram = this;
            var successCallback = args.success || function () {};
            var failCallback = args.failure || function (e) { console.error(e, e.stack); };

            this._readCRAI(dojo.hitch(this, function () {
                this._readCRAMheader(function () {
                    successCallback();
                }, failCallback);
            }), failCallback);
        },

        _readCRAI: function (successCallback, failCallback) {
            this.crai.fetch(dojo.hitch(this, function (header) {
                if (!header) {
                    dlog('No data read from CRAM index (CRAI) file');
                    failCallback('No data read from CRAM index (CRAI) file');
                    return;
                }

                if (!has('typed-arrays')) {
                    dlog('Web browser does not support typed arrays');
                    failCallback('Web browser does not support typed arrays');
                    return;
                }
                var Module = this.Module;

                var fp = Module.FS_createDataFile('/', 'cram.header', header, 1, 1, 1);
                var ret = Module.ccall('hts_open', 'number', ['string', 'string'], ['/cram.header', 'rb']);
                var header = Module.ccall('sam_hdr_read', 'number', ['number'], [ret]);
                console.log(header);
                var aln = Module.ccall('bam_init1', 'number', [], []);
                console.log(aln);
                // Assumes the struct starts on a 4-byte boundary
                var myNumber = Module.HEAPU32[header / 4];
                console.log(myNumber);
                // Assumes my_char_array is immediately after my_number with no padding
                var myCharArray = Module.HEAPU8.subarray(header + 4, header + 4 + 32);
                console.log(myCharArray);
                if (Module.ccall('sam_read1', 'number', ['number', 'number', 'number'], [ret, header, aln]) >= 0) {
                    var v = Module.ccall('bam_aux_get', 'number', ['number', 'string', 'character'], [aln, 'XA', 'A']);
                    console.log('v', v);
                    var x = Module.ccall('bam_aux2A', 'number', ['number'], [v]);
                    console.log('x', x);
                }
            }));
        }
    });
});
