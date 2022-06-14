import { SlpTransactionDetails, SlpTransactionType, LocalValidator, 
@jcramer
add nft1ParentId to NFT1 child graphs
2 years ago
2
         Utils, Slp, Primatives, SlpVersionType } from 'slpjs';
@jcramer
Updates:
3 years ago
3
import BigNumber from 'bignumber.js';
@jcramer
update bitbox version
3 years ago
4
import { BITBOX } from 'bitbox-sdk';
@jcramer
updates updates (still not functional for alpha)
3 years ago
5
import * as bitcore from 'bitcore-lib-cash';
@jcramer
update db commits in graceful shutdown during sync
3 years ago
6
import { SendTxnQueryResult, Query } from './query';
@jcramer
refactor statistics updates
3 years ago
7
import { Db } from './db';
@jcramer
update rpc with a class wrapper
3 years ago
8
import { RpcClient } from './rpc';
@jcramer
update bitbox version
3 years ago
9
import * as pQueue from 'p-queue';
10
import { DefaultAddOptions } from 'p-queue';
@jcramer
minor file naming to lowercase
3 years ago
11
import { SlpGraphManager } from './slpgraphmanager';
@jcramer
- improve double spend handling on block hash
3 years ago
12
import { CacheMap } from './cache';
@jcramer
1.0.0 release
2 years ago
13
import { SlpTransactionDetailsDbo, TokenUtxoStatus,
14
         BatonUtxoStatus, TokenBatonStatus, GraphTxn, TokenDBObject } from './interfaces';
@jcramer
refactored _graphTxns with new GraphMap class
3 years ago
15
import { GraphMap } from './graphmap';
@jcramer
Updates:
3 years ago
16

@jcramer
added queue system for improved controls over data
3 years ago
17
const sleep = (ms: number) => new Promise(resolve => setTimeout(resolve, ms));
18

@jcramer
update bitbox version
3 years ago
19
const bitbox = new BITBOX();
@jcramer
fix issue with nft1 children when they spend non-SLP outputs from val…
3 years ago
20
const slp = new Slp(bitbox);
@jcramer
Updates:
3 years ago
21

@jcramer
detect burns in non-SLP txns
2 years ago
22
import { slpUtxos } from './utxos';
23
const globalUtxoSet = slpUtxos();
24

@jcramer
updates:
2 years ago
25
import { PruneStack } from './prunestack';
@jcramer
add pruning stack
2 years ago
26

@jcramer
refactored _graphTxns with new GraphMap class
3 years ago
27
export class SlpTokenGraph {
@jcramer
various sync improvements
3 years ago
28

@jcramer
add lazy loading
3 years ago
29
    _tokenIdHex: string;
@jcramer
detect burns in non-SLP txns
2 years ago
30
    _tokenIdBuf: Buffer; // NOTE: will need to consider future interaction with lazy loading/garbage collection 
@jcramer
udpate for loading token graph state from db, then updating the graph
3 years ago
31
    _lastUpdatedBlock!: number;
@jcramer
updates:
3 years ago
32
    _tokenDetails: SlpTransactionDetails;
@jcramer
various sync improvements
3 years ago
33
    _blockCreated: number|null;
@jcramer
double spend fix WIP
3 years ago
34
    _mintBatonUtxo = "";
@jcramer
manage mint baton status
2 years ago
35
    _mintBatonStatus = TokenBatonStatus.UNKNOWN;
@jcramer
updates
2 years ago
36
    _nftParentId?: string;
@jcramer
1.0.0 release
2 years ago
37
    _graphTxns: GraphMap;
@jcramer
more updates in pruning and graph building
3 years ago
38
    _slpValidator = new LocalValidator(bitbox, async (txids) => {
@jcramer
updates
2 years ago
39
        // if (this._manager._bit.doubleSpendCache.has(txids[0])) {
40
        //     return [ Buffer.alloc(60).toString('hex') ];
41
        // }
@jcramer
wrap getRawTransaction in try-catch
3 years ago
42
        let txn;
43
        try {
@jcramer
updates
2 years ago
44
            txn = <string>await RpcClient.getRawTransaction(txids[0], false);
@jcramer
wrap getRawTransaction in try-catch
3 years ago
45
        } catch(err) {
46
            console.log(`[ERROR] Could not get transaction ${txids[0]} in local validator: ${err}`);
47
            return [ Buffer.alloc(60).toString('hex') ];
48
        }
49
        return [ txn ];
50
    }, console);
@jcramer
remove redundant RPC calls
3 years ago
51
    _network: string;
@jcramer
refactor statistics updates
3 years ago
52
    _db: Db;
@jcramer
sync/crawl updates
3 years ago
53
    _graphUpdateQueue: pQueue<DefaultAddOptions> = new pQueue({ concurrency: 1, autoStart: true });
@jcramer
fix graceful shutdown event
3 years ago
54
    _graphUpdateQueueOnIdle?: ((self: this) => Promise<void>) | null;
@jcramer
graph txn item updates and more logging
3 years ago
55
    _graphUpdateQueueNewTxids = new Set<string>();
@jcramer
Use pQueue for "updateTokenGraphFrom(...)"
3 years ago
56
    _manager: SlpGraphManager;
@jcramer
- improve double spend handling on block hash
3 years ago
57
    _startupTxoSendCache?: CacheMap<string, SpentTxos>;
@jcramer
updates for upfront validation
3 years ago
58
    _loadInitiated = false;
@jcramer
1.0.0 release
2 years ago
59
    _lazilyLoaded = false;
@jcramer
update db commits in graceful shutdown during sync
3 years ago
60
    _updateComplete = true;
61
    _isValid?: boolean;
@jcramer
1.0.0 release
2 years ago
62
    _tokenDbo: TokenDBObject|null;
@jcramer
updates:
3 years ago
63

@jcramer
1.0.0 release
2 years ago
64
    constructor(tokenDetails: SlpTransactionDetails, manager: SlpGraphManager, blockCreated: number|null, tokenDbo: TokenDBObject|null) {
@jcramer
updates:
3 years ago
65
        this._tokenDetails = tokenDetails;
66
        this._tokenIdHex = tokenDetails.tokenIdHex;
@jcramer
detect burns in non-SLP txns
2 years ago
67
        this._tokenIdBuf = Buffer.from(this._tokenIdHex, "hex");
@jcramer
prune validator caches
3 years ago
68
        this._graphTxns = new GraphMap(this);
@jcramer
1.0.0 release
2 years ago
69
        this._db = manager.db;
@jcramer
Use pQueue for "updateTokenGraphFrom(...)"
3 years ago
70
        this._manager = manager;
@jcramer
1.0.0 release
2 years ago
71
        this._network = manager._network;
@jcramer
various sync improvements
3 years ago
72
        this._blockCreated =  blockCreated;
@jcramer
1.0.0 release
2 years ago
73
        this._tokenDbo = tokenDbo;
@jcramer
updates updates (still not functional for alpha)
3 years ago
74
    }
@jcramer
updates:
3 years ago
75

@jcramer
extend GraphMap with stat methods
3 years ago
76
    get graphSize() {
77
        return this._graphTxns.size;
78
    }
79

@jcramer
e2e test updates
2 years ago
80
    public scanDoubleSpendTxids(txidToDelete: Set<string>): boolean {
@jcramer
extend GraphMap with stat methods
3 years ago
81
        for (let txid of txidToDelete) {
82
            if (this._graphTxns.has(txid)) {
83
                RpcClient.transactionCache.delete(txid);
@jcramer
graphmap api update
2 years ago
84
                this._graphTxns.delete(txid);
@jcramer
more refactoring
3 years ago
85
                this.commitToDb();
@jcramer
extend GraphMap with stat methods
3 years ago
86
                return true;
87
            }
88
        }
89
        return false
90
    }
91

@jcramer
refine labeling for spent outputs in issue (#62)
2 years ago
92
    markInvalidSlpOutputAsBurned(txo: string, burnedInTxid: string, blockIndex: number) {
@jcramer
detect burns in non-SLP txns
2 years ago
93
        let txid = txo.split(":")[0];
94
        let vout = Number.parseInt(txo.split(":")[1]);
95
        let gt = this._graphTxns.get(txid);
96
        if (gt) {
@jcramer
rc2 updates
2 years ago
97
            let o = gt.outputs.find(o => o.vout === vout && [TokenUtxoStatus.UNSPENT, BatonUtxoStatus.BATON_UNSPENT].includes(o.status));
@jcramer
detect burns in non-SLP txns
2 years ago
98
            if (o) {
99
                let batonVout;
100
                if ([SlpTransactionType.GENESIS, SlpTransactionType.MINT].includes(gt.details.transactionType)) {
101
                    batonVout = gt.details.batonVout;
102
                }
103
                if (batonVout === vout) {
@jcramer
refine labeling for spent outputs in issue (#62)
2 years ago
104
                    o.status = BatonUtxoStatus.BATON_SPENT_INVALID_SLP;
105
                    o.invalidReason = "Token baton output burned in an invalid SLP transaction";
@jcramer
manage mint baton status
2 years ago
106
                    this._mintBatonUtxo = "";
107
                    this._mintBatonStatus = TokenBatonStatus.DEAD_BURNED;
@jcramer
detect burns in non-SLP txns
2 years ago
108
                } else {
@jcramer
refine labeling for spent outputs in issue (#62)
2 years ago
109
                    o.status = TokenUtxoStatus.SPENT_INVALID_SLP;
110
                    o.invalidReason = "Output burned in an invalid SLP transaction";
@jcramer
detect burns in non-SLP txns
2 years ago
111
                }
112
                o.spendTxid = burnedInTxid;
@jcramer
simplify txn counting code
2 years ago
113
                this._graphTxns.setDirty(txid);
@jcramer
updates:
2 years ago
114
                if (gt.outputs.filter(o => [ TokenUtxoStatus.UNSPENT, BatonUtxoStatus.BATON_UNSPENT ].includes(o.status)).length === 0) {
115
                    let pruningStack = PruneStack()
116
                    pruningStack.addGraphTxidToPruningStack(blockIndex, this._tokenIdHex, txid);
117
                }
@jcramer
detect burns in non-SLP txns
2 years ago
118
                return true;
119
            }
120
        }
121
        return false;
122
    }
123

@jcramer
add pruning stack
2 years ago
124
    public considerTxidsForPruning(txids: string[], pruneHeight: number) {
125
        for (let txid of txids) {
126
            let gt = this._graphTxns.get(txid);
127
            if (gt) {
128
                let canBePruned = gt.outputs.filter(o => [ 
129
                                        BatonUtxoStatus.BATON_UNSPENT, 
130
                                        TokenUtxoStatus.UNSPENT
131
                                    ].includes(o.status)).length === 0;
132
                if (canBePruned) {
133
                    if (!gt.prevPruneHeight || pruneHeight >= gt.prevPruneHeight) {
134
                        gt.prevPruneHeight = pruneHeight;
@jcramer
simplify txn counting code
2 years ago
135
                        this._graphTxns.setDirty(txid);
@jcramer
add pruning stack
2 years ago
136
                    }
137
                }
138
            }
139
        }
140
    }
141

142
    public async commitToDb() {
143
        await this._db.graphItemsUpsert(this._graphTxns);
@jcramer
more refactoring
3 years ago
144
        this._updateComplete = true;
@jcramer
extend GraphMap with stat methods
3 years ago
145
    }
146

@jcramer
more refactoring
3 years ago
147
    public async validateTxid(txid: string) {
@jcramer
small refactor on validator call
3 years ago
148
        await this._slpValidator.isValidSlpTxid(txid, this._tokenIdHex);
149
        const validation = this._slpValidator.cachedValidations[txid];
@jcramer
1.0.0 release
2 years ago
150
        if (! validation.validity) {
@jcramer
small refactor on validator call
3 years ago
151
            delete this._slpValidator.cachedValidations[txid];
152
            delete this._slpValidator.cachedRawTransactions[txid];
153
        }
154
        return validation;
155
    }
156

@jcramer
more refactoring
3 years ago
157
    public async stop() {
@jcramer
update db commits in graceful shutdown during sync
3 years ago
158
        console.log(`[INFO] Stopping token graph ${this._tokenIdHex}, with ${this._graphTxns.size} loaded.`);
@jcramer
update shutdown and other minor updates
3 years ago
159

@jcramer
update db commits in graceful shutdown during sync
3 years ago
160
        if (this._graphUpdateQueue.pending || this._graphUpdateQueue.size) {
161
            console.log(`[INFO] Waiting on ${this._graphUpdateQueue.size} queue items.`);
@jcramer
shutdown may encounter already paused queue
2 years ago
162
            if (!this._graphUpdateQueue.isPaused) {
163
                await this._graphUpdateQueue.onIdle();
164
                this._graphUpdateQueue.pause();
165
                console.log(`[INFO] Graph update queue is idle and cleared with ${this._graphUpdateQueue.size} items and ${this._graphUpdateQueue.pending} pending.`);
166
            }
@jcramer
update shutdown and other minor updates
3 years ago
167
        }
168

@jcramer
add set for tracking dirty graph items
2 years ago
169
        let dirtyCount = this._graphTxns.DirtyCount;
@jcramer
update db commits in graceful shutdown during sync
3 years ago
170
        console.log(`[INFO] On stop there are ${dirtyCount} dirty items.`);
171
        if (dirtyCount > 0) {
@jcramer
more refactoring
3 years ago
172
            this.commitToDb();
@jcramer
update db commits in graceful shutdown during sync
3 years ago
173
        }
174

175
        while (this._graphUpdateQueueOnIdle !== undefined || !this._updateComplete) {
@jcramer
update shutdown and other minor updates
3 years ago
176
            console.log(`Waiting for UpdateStatistics to finish for ${this._tokenIdHex}`);
177
            await sleep(500);
178
        }
@jcramer
update db commits in graceful shutdown during sync
3 years ago
179
        console.log(`[INFO] Stopped token graph ${this._tokenIdHex}`);
@jcramer
graceful shutdown for SIGINT and SIGTERM
3 years ago
180
    }
181

@jcramer
add nft1ParentId to NFT1 child graphs
2 years ago
182
    public async setNftParentId() {
183
        if (this._tokenDetails.versionType === SlpVersionType.TokenVersionType1_NFT_Child) {
184
            let txnhex = await RpcClient.getRawTransaction(this._tokenIdHex);
185
            let tx = Primatives.Transaction.parseFromBuffer(Buffer.from(txnhex, 'hex'));
186
            let nftBurnTxnHex = await RpcClient.getRawTransaction(tx.inputs[0].previousTxHash);
187
            let nftBurnTxn = Primatives.Transaction.parseFromBuffer(Buffer.from(nftBurnTxnHex, 'hex'));
188
            let nftBurnSlp = slp.parseSlpOutputScript(Buffer.from(nftBurnTxn.outputs[0].scriptPubKey));
@jcramer
1.0.0 release
2 years ago
189

@jcramer
add nft1ParentId to NFT1 child graphs
2 years ago
190
            if (nftBurnSlp.transactionType === SlpTransactionType.GENESIS) {
191
                this._nftParentId = tx.inputs[0].previousTxHash;
@jcramer
1.0.0 release
2 years ago
192
            } else {
@jcramer
add nft1ParentId to NFT1 child graphs
2 years ago
193
                this._nftParentId = nftBurnSlp.tokenIdHex;
194
            }
@jcramer
add nftParentId property to child token graph
3 years ago
195
        }
196
    }
197

@jcramer
more refactoring
3 years ago
198
    public async IsValid(): Promise<boolean> {
@jcramer
update db commits in graceful shutdown during sync
3 years ago
199
        if (this._isValid || this._isValid === false) {
200
            return this._isValid;
201
        }
202
        this._isValid = await this._slpValidator.isValidSlpTxid(this._tokenIdHex);
203
        return this._isValid;
@jcramer
Updates:
3 years ago
204
    }
205

@jcramer
more refactoring
3 years ago
206
    public get IsLoaded(): boolean {
@jcramer
add lazy loading
3 years ago
207
        return this._graphTxns.size > 0;
208
    }
209

@jcramer
fix output status updates (#62)
2 years ago
210
    private async getMintBatonSpentOutputDetails({ txid, vout }: { txid: string; vout: number; }): Promise<MintSpendDetails> {
@jcramer
sync/crawl updates
3 years ago
211
        let spendTxnInfo: SendTxnQueryResult | {txid: string, block: number|null} | undefined
@jcramer
some formatting fixups
3 years ago
212
        if (this._startupTxoSendCache) {
@jcramer
sync/crawl updates
3 years ago
213
            spendTxnInfo = this._startupTxoSendCache.get(txid + ":" + vout);
@jcramer
update e2e tests
2 years ago
214
            if (spendTxnInfo) {
@jcramer
sync/crawl updates
3 years ago
215
                console.log("[INFO] Used _startupTxoSendCache data", txid, vout);
216
            }
217
        }
@jcramer
some formatting fixups
3 years ago
218
        if (!spendTxnInfo) {
@jcramer
sync/crawl updates
3 years ago
219
            spendTxnInfo = this._manager._bit._spentTxoCache.get(txid + ":" + vout); //this._liveTxoSpendCache.get(txid + ":" + vout);
@jcramer
update e2e tests
2 years ago
220
            if (spendTxnInfo) {
@jcramer
sync/crawl updates
3 years ago
221
                console.log("[INFO] Used bit._spentTxoCache data", txid, vout);
222
            }
223
        }
@jcramer
improve efficiency of graph processing
3 years ago
224
        // This is a backup to prevent bad data, it should rarely be used and should be removed in the future
@jcramer
sync/crawl updates
3 years ago
225
        if (!spendTxnInfo) {
226
            let res = await Query.queryForTxoInputAsSlpMint(txid, vout);
227
            if (res) {
228
                spendTxnInfo = { txid: res.txid!, block: res.block };
229
            }
230
        }
@jcramer
detect burns in non-SLP txns
2 years ago
231
        if (!spendTxnInfo) {
232
            throw Error(`Unable to locate spend details for output ${txid}:${vout}.`);
233
        }
234
        let validation = await this.validateTxid(spendTxnInfo.txid!);
@jcramer
rc5 updates:
2 years ago
235
        if (!validation) {
236
            throw Error(`SLP Validator is missing transaction ${spendTxnInfo.txid} for token ${this._tokenDetails.tokenIdHex}`);
237
        }
238
        if (validation.validity && validation.details!.transactionType === SlpTransactionType.MINT) {
239
            globalUtxoSet.delete(`${txid}:${vout}`);
240
            return { status: BatonUtxoStatus.BATON_SPENT_IN_MINT, txid: spendTxnInfo!.txid, invalidReason: null };
241
        } else if (validation.validity) {
@jcramer
detect burns in non-SLP txns
2 years ago
242
            this._mintBatonUtxo = '';
@jcramer
manage mint baton status
2 years ago
243
            this._mintBatonStatus = TokenBatonStatus.DEAD_BURNED;
244
            globalUtxoSet.delete(`${txid}:${vout}`);
@jcramer
rc5 updates:
2 years ago
245
            return { status: BatonUtxoStatus.BATON_SPENT_NOT_IN_MINT, txid: spendTxnInfo!.txid, invalidReason: "Baton was spent in a non-mint SLP transaction." };
246
        } else {
247
            this._mintBatonUtxo = '';
248
            this._mintBatonStatus = TokenBatonStatus.DEAD_BURNED;
@jcramer
fix output status updates (#62)
2 years ago
249

250
            const txnHex = await RpcClient.getRawTransaction(spendTxnInfo.txid!);
251
            const txn = Primatives.Transaction.parseFromBuffer(Buffer.from(txnHex, "hex"));
252

@jcramer
refine labeling for spent outputs in issue (#62)
2 years ago
253
            // SPENT_INVALID_SLP (bad OP_RETURN)          
@jcramer
fix output status updates (#62)
2 years ago
254
            try {
255
                slp.parseSlpOutputScript(Buffer.from(txn.outputs[0]!.scriptPubKey));
256
            } catch (_) {
@jcramer
refine labeling for spent outputs in issue (#62)
2 years ago
257
                return  { status: BatonUtxoStatus.BATON_SPENT_INVALID_SLP, txid: spendTxnInfo.txid!, invalidReason: "SLP baton output was spent in an invalid SLP transaction (bad SLP metadata)." }
258
            }
259

260
            // SPENT_INVALID_SLP (bad DAG)          
261
            if (! validation.validity) {
262
                return  { status: BatonUtxoStatus.BATON_SPENT_INVALID_SLP, txid: spendTxnInfo.txid!, invalidReason: "SLP baton output was spent in an invalid SLP transaction (bad DAG)." }
@jcramer
detect burns in non-SLP txns
2 years ago
263
            }
@jcramer
fix output status updates (#62)
2 years ago
264

265
            // MISSING_BCH_VOUT
266
            if (vout > txn.outputs.length-1) {
267
                return { status: BatonUtxoStatus.BATON_MISSING_BCH_VOUT, txid: spendTxnInfo.txid!, invalidReason: "SLP baton output has no corresponding BCH output." };
268
            }
269

@jcramer
refine labeling for spent outputs in issue (#62)
2 years ago
270
            throw Error(`Unhandled scenario for updating token baton output status (${txid}:${vout} in ${spendTxnInfo.txid})`);
@jcramer
Updates for minting baton state & statistics
3 years ago
271
        }
272
    }
273

@jcramer
fix output status updates (#62)
2 years ago
274
    private async getSpentOutputDetails({ txid, vout }: { txid: string; vout: number; }): Promise<SpendDetails> {
275
        let spendTxnInfo: SendTxnQueryResult | { txid: string, block: number|null } | undefined
@jcramer
some formatting fixups
3 years ago
276
        if (this._startupTxoSendCache) {
@jcramer
sync/crawl updates
3 years ago
277
            spendTxnInfo = this._startupTxoSendCache.get(txid + ":" + vout);
278
            if (spendTxnInfo) {
279
                console.log("[INFO] Used _startupTxoSendCache data", txid, vout);
280
            }
@jcramer
utilize global "txo->spend txid" cache
3 years ago
281
        }
@jcramer
some formatting fixups
3 years ago
282
        if (!spendTxnInfo) {
@jcramer
small refactor on validator call
3 years ago
283
            spendTxnInfo = this._manager._bit._spentTxoCache.get(txid + ":" + vout);
@jcramer
sync/crawl updates
3 years ago
284
            if (spendTxnInfo) {
285
                console.log("[INFO] Used bit._spentTxoCache spend data", txid, vout);
286
            }
@jcramer
utilize global "txo->spend txid" cache
3 years ago
287
        }
@jcramer
improve efficiency of graph processing
3 years ago
288
        // NOTE: This is a backup to prevent bad data, it should rarely be used and should be removed in the future
@jcramer
sync/crawl updates
3 years ago
289
        if (!spendTxnInfo) {
290
            let res = await Query.queryForTxoInputAsSlpSend(txid, vout);
291
            if (res) {
@jcramer
improve efficiency of graph processing
3 years ago
292
                console.log(`[DEBUG] OUTPUT INFO ADDED: ${txid}:${vout} -> ${res.txid}`);
@jcramer
sync/crawl updates
3 years ago
293
                spendTxnInfo = { txid: res.txid!, block: res.block };
294
            }
295
        }
@jcramer
detect burns in non-SLP txns
2 years ago
296
        if (!spendTxnInfo) {
297
            throw Error(`Unable to locate spend details for output ${txid}:${vout}.`);
298
        }
299
        let validation = await this.validateTxid(spendTxnInfo.txid!);
@jcramer
rc5 updates:
2 years ago
300
        if (!validation) {
301
            throw Error(`SLP Validator is missing transaction ${spendTxnInfo.txid} for token ${this._tokenDetails.tokenIdHex}`);
302
        }
303
        if (validation.validity && validation.details!.transactionType === SlpTransactionType.SEND) {
@jcramer
detect burns in non-SLP txns
2 years ago
304
            globalUtxoSet.delete(`${txid}:${vout}`);
@jcramer
rc5 updates:
2 years ago
305
            return { status: TokenUtxoStatus.SPENT_SAME_TOKEN, txid: spendTxnInfo!.txid, invalidReason: null };
306
        } else if (validation.validity) {
307
            globalUtxoSet.delete(`${txid}:${vout}`);
308
            return { status: TokenUtxoStatus.SPENT_NOT_IN_SEND, txid: spendTxnInfo!.txid, invalidReason: null };
309
        } else {
@jcramer
fix output status updates (#62)
2 years ago
310
            const txnHex = await RpcClient.getRawTransaction(spendTxnInfo.txid!);
311
            const txn = Primatives.Transaction.parseFromBuffer(Buffer.from(txnHex, "hex"));
312

@jcramer
refine labeling for spent outputs in issue (#62)
2 years ago
313
            // SPENT_INVALID_SLP (bad OP_RETURN)
@jcramer
fix output status updates (#62)
2 years ago
314
            try {
315
                slp.parseSlpOutputScript(Buffer.from(txn.outputs[0]!.scriptPubKey));
316
            } catch (_) {
@jcramer
refine labeling for spent outputs in issue (#62)
2 years ago
317
                return  { status: TokenUtxoStatus.SPENT_INVALID_SLP, txid: spendTxnInfo.txid!, invalidReason: "SLP output was spent in an invalid SLP transaction (bad SLP metadata)." }
318
            }
319

320
            // SPENT_INVALID_SLP (bad DAG)
321
            if (! validation.validity) {
322
                return  { status: TokenUtxoStatus.SPENT_INVALID_SLP, txid: spendTxnInfo.txid!, invalidReason: "SLP output was spent in an invalid SLP transaction (bad DAG)." }
@jcramer
updates:
3 years ago
323
            }
@jcramer
fix output status updates (#62)
2 years ago
324

325
            // MISSING_BCH_VOUT
326
            if (vout > txn.outputs.length-1) {
327
                return { status: TokenUtxoStatus.MISSING_BCH_VOUT, txid: spendTxnInfo.txid!, invalidReason: "SLP output has no corresponding BCH output." };
328
            }
329

@jcramer
refine labeling for spent outputs in issue (#62)
2 years ago
330
            throw Error(`Unhandled scenario for updating token output status (${txid}${vout} in ${spendTxnInfo.txid})`);
@jcramer
update utxo update strategy
3 years ago
331
        }
@jcramer
updates:
3 years ago
332
    }
@jcramer
Updates:
3 years ago
333

@jcramer
updates
2 years ago
334
    public async queueAddGraphTransaction({ txid }: { txid: string }): Promise<void> {
@jcramer
updates for upfront validation
3 years ago
335
        let self = this;
336

@jcramer
1.0.0 release
2 years ago
337
        while (this._loadInitiated && !this.IsLoaded && this._tokenIdHex !== txid) {
@jcramer
updates for upfront validation
3 years ago
338
            console.log(`Waiting for token ${this._tokenIdHex} to finish loading...`);
339
            await sleep(250);
340
        }
341

342
        if (!this._loadInitiated && !this.IsLoaded) {
343
            this._loadInitiated = true;
@jcramer
update db commits in graceful shutdown during sync
3 years ago
344
            return this._graphUpdateQueue.add(async () => {
@jcramer
graph txn item updates and more logging
3 years ago
345
                console.log(`[INFO] (queueTokenGraphUpdateFrom) Initiating graph for ${txid}`);
@jcramer
updates
2 years ago
346
                await self.addGraphTransaction({ txid });
@jcramer
more updates in pruning and graph building
3 years ago
347
            });
@jcramer
add lazy loading
3 years ago
348
        } else {
@jcramer
update db commits in graceful shutdown during sync
3 years ago
349
            return this._graphUpdateQueue.add(async () => {
@jcramer
graph txn item updates and more logging
3 years ago
350
                console.log(`[INFO] (queueTokenGraphUpdateFrom) Updating graph from ${txid}`);
@jcramer
updates
2 years ago
351
                await self.addGraphTransaction({ txid });
@jcramer
add lazy loading
3 years ago
352
            });
353
        }
@jcramer
Use pQueue for "updateTokenGraphFrom(...)"
3 years ago
354
    }
355

@jcramer
more refactoring
3 years ago
356
    public async addGraphTransaction({ txid, processUpToBlock, blockHash }: { txid: string; processUpToBlock?: number; blockHash?: Buffer; }): Promise<boolean|null> {
@jcramer
prune validator caches
3 years ago
357
        if (this._graphTxns.has(txid)) {
358
            let gt = this._graphTxns.get(txid)!;
@jcramer
update e2e tests
2 years ago
359
            if (blockHash) {
@jcramer
prune validator caches
3 years ago
360
                gt.blockHash = blockHash;
@jcramer
simplify txn counting code
2 years ago
361
                this._graphTxns.setDirty(txid);
@jcramer
prune validator caches
3 years ago
362
            }
@jcramer
- improve double spend handling on block hash
3 years ago
363
            return true;
364
        }
365

@jcramer
Fixed bug in token specific validation impacting circulating supply math
3 years ago
366
        let isValid = await this._slpValidator.isValidSlpTxid(txid, this._tokenDetails.tokenIdHex);
@jcramer
token output updates:
3 years ago
367
        let txnSlpDetails = this._slpValidator.cachedValidations[txid].details;
@jcramer
Graph search updates
3 years ago
368
        let txn: bitcore.Transaction = new bitcore.Transaction(await this._slpValidator.retrieveRawTransaction(txid));
@jcramer
incremental updates:
3 years ago
369

@jcramer
graph txn item updates and more logging
3 years ago
370
        if (!txnSlpDetails) {
@jcramer
fix block hash in block crawl in block notifications
2 years ago
371
            console.log("[WARN] addGraphTransaction: No token details for:", txid);
@jcramer
incremental updates:
3 years ago
372
            return false;
@jcramer
completed txo accounting for slp txns
3 years ago
373
        }
@jcramer
incremental updates:
3 years ago
374

@jcramer
do not prune if prevPruneHeight is less than current prune height
3 years ago
375
        let graphTxn: GraphTxn = {
376
            details: txnSlpDetails,
377
            outputs: [],
378
            inputs: [],
379
            blockHash: blockHash ? blockHash : null,
380
            prevPruneHeight: null
@jcramer
fix log line (this count is unprunned txn in mem)
3 years ago
381
        };
382

@jcramer
fix typo
2 years ago
383
        console.log(`[INFO] Unpruned txn count: ${this._graphTxns.size} (token: ${this._tokenIdHex})`);
@jcramer
completed txo accounting for slp txns
3 years ago
384

@jcramer
graph txn item updates and more logging
3 years ago
385
        // Update parent items (their output statuses) and add contributing SLP inputs
@jcramer
various sync improvements
3 years ago
386
        if (txid !== this._tokenIdHex) {
@jcramer
don't update input txid if already visited
3 years ago
387
            let visited = new Set<string>();
@jcramer
manage mint baton status
2 years ago
388

389
            // update parent input details
@jcramer
graph txn item updates and more logging
3 years ago
390
            for (let i of txn.inputs) {
@jcramer
update utxo update strategy
3 years ago
391
                let previd = i.prevTxId.toString('hex');
@jcramer
graph txn item updates and more logging
3 years ago
392

393
                if (this._graphTxns.has(previd)) {
@jcramer
improve efficiency of graph processing
3 years ago
394
                    let ptxn = this._graphTxns.get(previd)!;
@jcramer
simplify txn counting code
2 years ago
395
                    this._graphTxns.setDirty(previd);
@jcramer
improve efficiency of graph processing
3 years ago
396
                    // update the parent's output items
@jcramer
fix block hash in block crawl in block notifications
2 years ago
397
                    console.log("[INFO] addGraphTransaction: update the status of the input txns' outputs");
@jcramer
don't update input txid if already visited
3 years ago
398
                    if (!visited.has(previd)) {
@jcramer
fix prev commit
3 years ago
399
                        visited.add(previd);
@jcramer
improve efficiency of graph processing
3 years ago
400
                        //await this.updateTokenGraphAt({ txid: previd, isParentInfo: {  }, processUpToBlock });
401
                        let gtos = ptxn!.outputs;
402
                        let prevOutpoints = txn.inputs.filter(i => i.prevTxId.toString('hex') === previd).map(i => i.outputIndex);
403
                        for (let vout of prevOutpoints) {
404
                            let spendInfo: SpendDetails|MintSpendDetails;
405
                            if ([SlpTransactionType.GENESIS, SlpTransactionType.MINT].includes(ptxn!.details.transactionType) &&
@jcramer
fix typo
3 years ago
406
                                ptxn.details.batonVout === vout) {
@jcramer
fix output status updates (#62)
2 years ago
407
                                    spendInfo = await this.getMintBatonSpentOutputDetails({ txid: previd, vout });
@jcramer
improve efficiency of graph processing
3 years ago
408
                            } else {
@jcramer
fix output status updates (#62)
2 years ago
409
                                spendInfo = await this.getSpentOutputDetails({ txid: previd, vout });
@jcramer
improve efficiency of graph processing
3 years ago
410
                            }
411
                            let o = gtos.find(o => o.vout === vout);
412
                            if (o) {
413
                                o.spendTxid = txid;
414
                                o.status = spendInfo.status;
415
                                o.invalidReason = spendInfo.invalidReason;
416
                            }
417
                        }
@jcramer
add pruning stack
2 years ago
418
                        if (processUpToBlock && gtos.filter(o => [ TokenUtxoStatus.UNSPENT, 
419
                                                                    BatonUtxoStatus.BATON_UNSPENT ].includes(o.status)).length === 0) 
420
                        {
@jcramer
updates:
2 years ago
421
                            let pruningStack = PruneStack();
@jcramer
add pruning stack
2 years ago
422
                            pruningStack.addGraphTxidToPruningStack(processUpToBlock, this._tokenIdHex, previd);
423
                        }
@jcramer
don't update input txid if already visited
3 years ago
424
                    }
@jcramer
improve efficiency of graph processing
3 years ago
425

@jcramer
fix issue with nft1 children when they spend non-SLP outputs from val…
3 years ago
426
                    // add the current input item to the current graphTxn object
@jcramer
more updates in pruning and graph building
3 years ago
427
                    let inputTxn = this._graphTxns.get(previd)!;
428
                    let o = inputTxn.outputs.find(o => o.vout === i.outputIndex);
@jcramer
graph txn item updates and more logging
3 years ago
429
                    if (o) {
@jcramer
update utxo update strategy
3 years ago
430
                        graphTxn.inputs.push({
431
                            txid: i.prevTxId.toString('hex'),
432
                            vout: i.outputIndex,
433
                            slpAmount: o.slpAmount,
@jcramer
set status for tokens missing bch outputs
2 years ago
434
                            address: o.address!,
@jcramer
set bchSatoshis to null when addr is null
2 years ago
435
                            bchSatoshis: o.bchSatoshis!
@jcramer
more updates in pruning and graph building
3 years ago
436
                        });
@jcramer
simplify txn counting code
2 years ago
437
                        this._graphTxns.setDirty(previd);
@jcramer
update utxo update strategy
3 years ago
438
                    }
439
                }
@jcramer
graph txn item updates and more logging
3 years ago
440
            }
@jcramer
manage mint baton status
2 years ago
441

@jcramer
updates
2 years ago
442
            if (graphTxn.inputs.length === 0) {
@jcramer
update warning messages for missing inputs
2 years ago
443
                console.log(`[WARN] Cannot have a SEND or MINT transaction without any input (${txid}).`);
@jcramer
remove throw statement preventing SLPDB from running
2 years ago
444
                //throw Error("Cannot have a SEND or MINT transaction without any input.");
@jcramer
updates
2 years ago
445
            }
@jcramer
manage mint baton status
2 years ago
446
        }
447

448
        if (!isValid) {
449
            console.log("[WARN] addGraphTransaction: Not valid token transaction:", txid);
450
            this.mempoolCommitToDb({});
451
            return false;
@jcramer
update utxo update strategy
3 years ago
452
        }
453

@jcramer
add EXCESS_INPUT_BURNED output status
3 years ago
454
        // Create or update SLP graph outputs for each valid SLP output
@jcramer
various sync improvements
3 years ago
455
        if (graphTxn.details.transactionType === SlpTransactionType.GENESIS || graphTxn.details.transactionType === SlpTransactionType.MINT) {
456
            if (graphTxn.details.genesisOrMintQuantity!.isGreaterThanOrEqualTo(0)) {
457
                let address = this.getAddressStringFromTxnOutput(txn, 1);
@jcramer
detect burns in non-SLP txns
2 years ago
458
                globalUtxoSet.set(`${txid}:${1}`, this._tokenIdBuf.slice());
@jcramer
various sync improvements
3 years ago
459
                graphTxn.outputs.push({
460
                    address: address,
461
                    vout: 1,
@jcramer
follow up prev commit
2 years ago
462
                    bchSatoshis: txn.outputs.length > 1 ? txn.outputs[1].satoshis : address ? 0 : null, 
@jcramer
simplify type structure
2 years ago
463
                    slpAmount: graphTxn.details.genesisOrMintQuantity! as BigNumber,
@jcramer
manage mint baton status
2 years ago
464
                    spendTxid: null,
@jcramer
set status for tokens missing bch outputs
2 years ago
465
                    status: address ? TokenUtxoStatus.UNSPENT : TokenUtxoStatus.MISSING_BCH_VOUT,
466
                    invalidReason: address ? null : "Transaction is missing output."
@jcramer
various sync improvements
3 years ago
467
                });
@jcramer
manage mint baton status
2 years ago
468
                if (txnSlpDetails.batonVout) {
469
                    this._mintBatonStatus = TokenBatonStatus.ALIVE;
470
                    this._mintBatonUtxo = `${txid}:${txnSlpDetails.batonVout}`;
@jcramer
fix address bug for minting batons
2 years ago
471
                    let address = this.getAddressStringFromTxnOutput(txn, txnSlpDetails.batonVout);
@jcramer
detect burns in non-SLP txns
2 years ago
472
                    globalUtxoSet.set(`${txid}:${txnSlpDetails.batonVout}`, this._tokenIdBuf.slice());
@jcramer
Updates for minting baton state & statistics
3 years ago
473
                    graphTxn.outputs.push({
474
                        address: address,
@jcramer
various sync improvements
3 years ago
475
                        vout: txnSlpDetails.batonVout,
@jcramer
set bchSatoshis to null when addr is null
2 years ago
476
                        bchSatoshis: txnSlpDetails.batonVout < txn.outputs.length ? txn.outputs[txnSlpDetails.batonVout].satoshis : address ? 0 : null, 
@jcramer
various sync improvements
3 years ago
477
                        slpAmount: new BigNumber(0),
@jcramer
manage mint baton status
2 years ago
478
                        spendTxid: null,
@jcramer
set status for tokens missing bch outputs
2 years ago
479
                        status: address ? BatonUtxoStatus.BATON_UNSPENT : BatonUtxoStatus.BATON_MISSING_BCH_VOUT,
480
                        invalidReason: address ? null : "Transaction is missing output."
@jcramer
improve efficiency of graph processing
3 years ago
481
                    });
@jcramer
manage mint baton status
2 years ago
482
                } else if (txnSlpDetails.batonVout === null) {
483
                    this._mintBatonUtxo = "";
484
                    this._mintBatonStatus = TokenBatonStatus.DEAD_ENDED;
@jcramer
various sync improvements
3 years ago
485
                }
486
            }
487
        }
488
        else if(graphTxn.details.sendOutputs!.length > 0) {
489
            let slp_vout = 0;
490
            for (let output of graphTxn.details.sendOutputs!) {
@jcramer
manage mint baton status
2 years ago
491
                if (output.isGreaterThanOrEqualTo(0)) {
@jcramer
various sync improvements
3 years ago
492
                    if (slp_vout > 0) {
493
                        let address = this.getAddressStringFromTxnOutput(txn, slp_vout);
@jcramer
detect burns in non-SLP txns
2 years ago
494
                        globalUtxoSet.set(`${txid}:${slp_vout}`, this._tokenIdBuf.slice());
@jcramer
completed txo accounting for slp txns
3 years ago
495
                        graphTxn.outputs.push({
updates for preventing crash on non-P2SH multisig
3 years ago
496
                            address: address,
@jcramer
various sync improvements
3 years ago
497
                            vout: slp_vout,
@jcramer
set bchSatoshis to null when addr is null
2 years ago
498
                            bchSatoshis: slp_vout < txn.outputs.length ? txn.outputs[slp_vout].satoshis : address ? 0 : null, 
@jcramer
various sync improvements
3 years ago
499
                            slpAmount: graphTxn.details.sendOutputs![slp_vout],
@jcramer
set status for tokens missing bch outputs
2 years ago
500
                            spendTxid: null,
501
                            status: address ? TokenUtxoStatus.UNSPENT : TokenUtxoStatus.MISSING_BCH_VOUT,
502
                            invalidReason: address ? null : "Transaction is missing output."
@jcramer
improve efficiency of graph processing
3 years ago
503
                        });
@jcramer
completed txo accounting for slp txns
3 years ago
504
                    }
@jcramer
updates:
3 years ago
505
                }
@jcramer
various sync improvements
3 years ago
506
                slp_vout++;
@jcramer
Graph search updates
3 years ago
507
            }
@jcramer
various sync improvements
3 years ago
508
        }
509
        else {
510
            console.log("[WARNING]: Transaction is not valid or is unknown token type!", txid);
@jcramer
updates:
3 years ago
511
        }
@jcramer
Updates:
3 years ago
512

@jcramer
fix bug with tracking burns for excesss inputs
3 years ago
513
        // check for possible inputs burned due to outputs < inputs
@jcramer
various sync improvements
3 years ago
514
        if (SlpTransactionType.GENESIS !== graphTxn.details.transactionType) {
@jcramer
fix bug with tracking burns for excesss inputs
3 years ago
515
            let outputQty = graphTxn.outputs.reduce((a, c) => a.plus(c.slpAmount), new BigNumber(0));
@jcramer
various sync improvements
3 years ago
516
            let inputQty = graphTxn.inputs.reduce((a, c) => a.plus(c.slpAmount), new BigNumber(0));
517
            if (outputQty.isGreaterThan(inputQty) && SlpTransactionType.MINT !== graphTxn.details.transactionType) {
@jcramer
update warning messages for missing inputs
2 years ago
518
                console.log(`[WARN] Graph item cannot have inputs less than outputs (txid: ${txid}, inputs: ${inputQty.toFixed()} | ${graphTxn.inputs.length}, outputs: ${outputQty.toFixed()} | ${graphTxn.outputs.length}).`);
@jcramer
remove throw statement preventing SLPDB from running
2 years ago
519
                //throw Error(`Graph item cannot have inputs less than outputs (txid: ${txid}, inputs: ${inputQty.toFixed()} | ${graphTxn.inputs.length}, outputs: ${outputQty.toFixed()} | ${graphTxn.outputs.length}).`);
@jcramer
various sync improvements
3 years ago
520
            }
521
            if (inputQty.isGreaterThan(outputQty)) {
@jcramer
fix bug with tracking burns for excesss inputs
3 years ago
522
                graphTxn.outputs.push(<any>{
523
                    slpAmount: inputQty.minus(outputQty),
524
                    status: TokenUtxoStatus.EXCESS_INPUT_BURNED
@jcramer
improve efficiency of graph processing
3 years ago
525
                });
@jcramer
fix bug with tracking burns for excesss inputs
3 years ago
526
            }
527
        }
528

@jcramer
graph txn item updates and more logging
3 years ago
529
        if(!processUpToBlock) {
@jcramer
add more caching for startup process
3 years ago
530
            this._lastUpdatedBlock = this._manager._bestBlockHeight; //await this._rpcClient.getBlockCount();
@jcramer
graph txn item updates and more logging
3 years ago
531
        } else {
@jcramer
minor update & refactor:
3 years ago
532
            this._lastUpdatedBlock = processUpToBlock;
@jcramer
graph txn item updates and more logging
3 years ago
533
        }
@jcramer
updates:
3 years ago
534

@jcramer
simplify txn counting code
2 years ago
535
        this._graphTxns.setDirty(txid, graphTxn);
@jcramer
various sync improvements
3 years ago
536

@jcramer
more refactoring
3 years ago
537
        if (!blockHash) {
538
            this.mempoolCommitToDb({ zmqTxid: txid });
539
        }
540

@jcramer
Updates:
3 years ago
541
        return true;
542
    }
543

@jcramer
rc5 updates:
2 years ago
544
    public async removeGraphTransaction({ txid }: { txid: string }) {
545
        if (!this._graphTxns.has(txid)) {
546
            return;
547
        }
548

549
        // update status of inputs to UNSPENT or BATON_UNSPENT 
550
        let gt = this._graphTxns.get(txid)!;
551
        for (let input of gt.inputs) {
552
            let gti = this._graphTxns.get(input.txid);
553
            if (gti) {
554
                let outs = gti.outputs.filter(o => o.spendTxid === txid);
555
                outs.forEach(o => {
556
                    if ([SlpTransactionType.GENESIS, SlpTransactionType.MINT].includes(gti!.details.transactionType) && 
557
                        o.vout === gti!.details.batonVout) 
558
                    {
559
                        o.spendTxid = null;
560
                        o.status = BatonUtxoStatus.BATON_UNSPENT;
561
                        globalUtxoSet.set(`${txid}:${o.vout}`, this._tokenIdBuf.slice());
@jcramer
fix wrong txid bug
2 years ago
562
                        this._graphTxns.setDirty(input.txid);
@jcramer
rc5 updates:
2 years ago
563
                    } else {
564
                        o.spendTxid = null;
565
                        o.status = TokenUtxoStatus.UNSPENT;
566
                        globalUtxoSet.set(`${txid}:${o.vout}`, this._tokenIdBuf.slice());
@jcramer
fix wrong txid bug
2 years ago
567
                        this._graphTxns.setDirty(input.txid);
@jcramer
rc5 updates:
2 years ago
568
                    }
569
                });
570
            }
571
        }
@jcramer
graphmap api update
2 years ago
572
        this._graphTxns.delete(txid);
@jcramer
rc5 updates:
2 years ago
573
    }
574

@jcramer
remove vendor.d.ts in favor of slpjs
3 years ago
575
    private getAddressStringFromTxnOutput(txn: bitcore.Transaction, outputIndex: number) {
@jcramer
fix for prev commit 43bda1b
3 years ago
576
        let address;
577
        try {
578
            address = Utils.toSlpAddress(bitbox.Address.fromOutputScript(txn.outputs[outputIndex]._scriptBuffer, this._network));
579
        }
580
        catch (_) {
581
            try {
582
                address = 'scriptPubKey:' + txn.outputs[outputIndex]._scriptBuffer.toString('hex');
583
            }
584
            catch (_) {
@jcramer
set status for tokens missing bch outputs
2 years ago
585
                address = null;
@jcramer
fix for prev commit 43bda1b
3 years ago
586
            }
587
        }
588
        return address;
589
    }
590

@jcramer
manage mint baton status
2 years ago
591
    private async mempoolCommitToDb({ zmqTxid }: { zmqTxid?: string }): Promise<void> {
@jcramer
graph txn item updates and more logging
3 years ago
592
        if (zmqTxid) {
593
            this._graphUpdateQueueNewTxids.add(zmqTxid);
594
        }
595
        if (!this._graphUpdateQueueOnIdle) {
@jcramer
update db commits in graceful shutdown during sync
3 years ago
596
            this._updateComplete = false;
597
            this._graphUpdateQueueOnIdle = async (self: SlpTokenGraph) => {
@jcramer
extend GraphMap with stat methods
3 years ago
598
                if (self._graphUpdateQueue.size !== 0 || self._graphUpdateQueue.pending !== 0) {
599
                    await self._graphUpdateQueue.onIdle();
600
                }
@jcramer
move pause location
2 years ago
601
                self._graphUpdateQueue.pause();
@jcramer
update db commits in graceful shutdown during sync
3 years ago
602
                let txidToUpdate = Array.from(self._graphUpdateQueueNewTxids);
603
                self._graphUpdateQueueNewTxids.clear();
@jcramer
fix graceful shutdown event
3 years ago
604
                self._graphUpdateQueueOnIdle = null;
@jcramer
extend GraphMap with stat methods
3 years ago
605
                self._updateComplete = false;
@jcramer
more refactoring
3 years ago
606
                await self.commitToDb();
@jcramer
graph txn item updates and more logging
3 years ago
607
                while (txidToUpdate.length > 0) {
@jcramer
update db commits in graceful shutdown during sync
3 years ago
608
                    await self._manager.publishZmqNotificationGraphs(txidToUpdate.pop()!);
@jcramer
graph txn item updates and more logging
3 years ago
609
                }
@jcramer
fix graceful shutdown event
3 years ago
610
                self._graphUpdateQueueOnIdle = undefined;
@jcramer
various sync improvements
3 years ago
611
                self._graphUpdateQueue.start();
@jcramer
more updates in pruning and graph building
3 years ago
612
                return;
613
            }
@jcramer
various sync improvements
3 years ago
614
            return this._graphUpdateQueueOnIdle(this); // Do not await this
@jcramer
don't update with pending queue item
3 years ago
615
        }
@jcramer
various sync improvements
3 years ago
616
        return;
617
    }
618

@jcramer
rc5 updates:
2 years ago
619
    // static FormatUnixToDateString(unix_time: number): string {
620
    //     var date = new Date(unix_time*1000);
621
    //     return date.toISOString().replace("T", " ").replace(".000Z", "")
622
    // }
@jcramer
update fixMissingTokenTimestamps
3 years ago
623

@jcramer
more updates in pruning and graph building
3 years ago
624
    public static MapDbTokenDetailsFromDbo(details: SlpTransactionDetailsDbo, decimals: number): SlpTransactionDetails {
@jcramer
updates:
3 years ago
625

626
        let genesisMintQty = new BigNumber(0);
627
        if(details.genesisOrMintQuantity)
@jcramer
Patch previous commit
3 years ago
628
            genesisMintQty = new BigNumber(details.genesisOrMintQuantity.toString()).multipliedBy(10**decimals);
@jcramer
updates:
3 years ago
629

@jcramer
fix map from DBO issue (o.dividedBy issue)
3 years ago
630
        let sendOutputs: BigNumber[] = [];
@jcramer
updates:
3 years ago
631
        if(details.sendOutputs)
@jcramer
Patch previous commit
3 years ago
632
            sendOutputs = details.sendOutputs.map(o => o = <any>new BigNumber(o.toString()).multipliedBy(10**decimals));
@jcramer
updates:
3 years ago
633

@uvhw
udpate for loading token graph state from db, then updating the graph
3 years ago
634
        let res = {
635
            decimals: details.decimals,
636
            tokenIdHex: details.tokenIdHex,
@jcramer
update 0.10.0:
3 years ago
637
            timestamp: details.timestamp!,
@jcramer
udpate for loading token graph state from db, then updating the graph
3 years ago
638
            transactionType: details.transactionType,
639
            versionType: details.versionType,
640
            documentUri: details.documentUri,
@jcramer
fix buffer to hex conversion in DBO
3 years ago
641
            documentSha256: details.documentSha256Hex ? Buffer.from(details.documentSha256Hex, 'hex') : null,
@uvhw
udpate for loading token graph state from db, then updating the graph
3 years ago
642
            symbol: details.symbol,
643
            name: details.name,
644
            batonVout: details.batonVout,
645
            containsBaton: details.containsBaton,
@uvhw
updates:
3 years ago
646
            genesisOrMintQuantity: details.genesisOrMintQuantity ? genesisMintQty : null,
@uvhw
fix map from DBO issue (o.dividedBy issue)
3 years ago
647
            sendOutputs: details.sendOutputs ? sendOutputs as any as BigNumber[] : null
@uvhw
udpate for loading token graph state from db, then updating the graph
3 years ago
648
        }
649

650
        return res;
@uvhw
added README instructions for alpha testing
3 years ago
651
    }
@uvhw
udpate for loading token graph state from db, then updating the graph
3 years ago
652
}
653

@uvhw
more refactoring
3 years ago
654
// export interface AddressBalance {
655
//     token_balance: BigNumber; 
656
//     satoshis_balance: number;
657
// }
@uvhw
new token statistics preview
3 years ago
658

@uvhw
updates:
3 years ago
659
interface SpendDetails {
@uvhw
Updates for minting baton state & statistics
3 years ago
660
    status: TokenUtxoStatus;
@uvhw
updates:
3 years ago
661
    txid: string|null;
@uvhw
handle missing case of missing BCH output
3 years ago
662
    invalidReason: string|null;
@uvhw
updates:
3 years ago
663
}
664

@uvhw
Updates for minting baton state & statistics
3 years ago
665
interface MintSpendDetails {
666
    status: BatonUtxoStatus;
667
    txid: string|null;
668
    invalidReason: string|null;
@uvhw
double spend fix WIP
3 years ago
669
}
670

@uvhw
more refactoring
3 years ago
671
interface SpentTxos {
@uvhw
double spend fix WIP
3 years ago
672
    txid: string;
673
    block: number|null;
@uvhw
more refactoring
3 years ago
674
    blockHash: Buffer|null;
@uvhw
double spend fix WIP
13 years ago
675
} 
              // @jcramer= uvhwuvhw
