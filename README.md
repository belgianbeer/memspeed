# memspeed

**日本語** | [English](#english)

`memcpy(3)` のスループットを測るだけの、ごく小さなメモリ帯域ベンチマークです。
本体は `memspeed.c` 1ファイル、依存は libc のみです。

```
Written by Masato Minda <minmin@wide.ad.jp>
```

## ビルド

```sh
make			# memspeed が生成される（既定で -O2）
make test		# 代表的なケースを数回実行
make testnoopt		# 同じケースを最適化なしで実行し、見比べる
make clean
```

`CFLAGS` の既定は `-O2` です。コピーループはバッファの末尾バイトを `volatile`
修飾したポインタ経由でも読んでいて、この読み取りはコンパイラに消せないため、
最適化されても計測対象の `memcpy()` はそのまま残ります（下の「実装上の注意」参照）。
ただし最適化のレベルそのものはコピーの「され方」に効いてくるので、素朴な
ビルドと見比べる口として `make testnoopt`（`CFLAGS` を空にした同じソースで
同じケースを回す）を残してあります。警告チェックをしたいときは、コマンド
ラインの `CFLAGS` が既定値をまるごと置き換えるため
`make CFLAGS='-O2 -Wall -Wextra -pedantic'` としてください（現状、警告は
出ません）。

## 使い方

```
Usage: memspeed [-offset] size count
	-offset: shift the destination by offset bytes
	   size: allocate size (k/KB, m/MB, g/GB; a bare number is MB)
	  count: repeat count
```

- `size`: 1回のコピー量。接尾辞なしは MB、`k`/`m`/`g`（大文字可、`b` 付きも可）で
  KB/MB/GB を指定します。
- `count`: そのコピーを繰り返す回数。
- `-offset`: 転送先を先頭から N バイトずらします。アライメント非一致の
  ペナルティを測るためのオプションです。

```sh
$ ./memspeed 10 10
copy 10 MB at 10 times (total 100 MB + 0 KB) in 0.022838 sec  4379 MB/sec

$ ./memspeed 128k 1000
copy 128 KB at 1000 times (total 128 MB + 0 KB) in 0.005626 sec  22752 MB/sec

$ ./memspeed -8 4 100
copy 4 MB at 100 times (total 400 MB + 0 KB) in 0.062776 sec  6372 MB/sec
```

終了コードは、正常終了が 0、引数の誤り・メモリ確保失敗・`-h`/`--help` が 1 です。
エラーメッセージは標準エラーに出力するので、結果行（標準出力）だけを
パイプで処理できます。

## 単位について

すべて 1000 基準です（1 KB = 1000 バイト、1 MB = 1000 KB）。
したがって末尾の `MB/sec` は「転送総バイト数 ÷ 経過秒 ÷ 1,000,000」であり、
2 進接頭辞（MiB/sec）ではありません。

## 実装上の注意

- **計測前にバッファを非ゼロパターン（0xA5）で埋めます。** `malloc` 直後の
  未触碰ページはカーネルの共有ゼロページで満たされていることが多く、
  ゼロクリアのままだと実際のメモリを触らずにコピーが済んでしまい、
  見かけの速度が実機より大きく出ます。
- **経過時間は可能なら `CLOCK_MONOTONIC`（ナノ秒分解能）で取ります。**
  マイクロ秒分解能の壁時計だけでは、短い実行で経過 0 秒となり、旧版はそこで
  0 除算をしていました。経過が 0 秒だった場合、速度欄は `N/A` になり、
  標準エラーに「size か count を大きくする」案内を出します。
  1 ミリ秒未満しか計測できなかった場合は別の注意文を出します。
- 分解能が不足すると感じる場合は `size` または `count` を大きくしてください。
- **最適化を有効にしても計測対象は消えません。** コピーだけのループは、コンパイラ
  から見れば「毎回同じ値を書き込んで、誰も読まない」処理なので、放っておくと
  `-O2` でループごと削除されます。実際に旧版は `cc -O2` ビルドで経過 0 秒・
  速度欄 `N/A` になっていました。現在は各回のコピーのあとで、転送元と転送先の
  末尾バイトを `volatile` 修飾ポインタ経由で読み合わせています。`volatile` 経由の
  読み取りは側効果なので消せず、しかも転送先の値はコピーが実際に走らなければ
  転送元と一致しないため、コンパイラはコピーを残すしかありません。万一ちがって
  いた場合は `copy mismatch` を標準エラーに出してループを抜け、その時点で
  完了した回数から総量を計算します。
  既定のビルドはこの `-O2` です。`make testnoopt` とすれば同じケースを最適化
  なしで測れて、これまで比べた範囲では両者の数値に差は出ていません。
- 極端に大きい `size` を指定すると、OS のオーバーコミットによりプロセスが
  強制終了されることがあります（`Can't alloc memory` を待たずに kill される
  ことがあります）。

## ポートビルドメモ

- C99 以降（`stdint.h` と `strtoull()` を使います）。
- macOS / Linux / Solaris で `$(CC)` に任せるだけの素朴な Makefile です。既定の
  `-O2` は主要コンパイラならどれも受け付けますが、外したいときは
  `make CFLAGS=` で最適化なしのビルドになります。
- `clock_gettime()` が libc に含まれない古いシステムでは `LDLIBS=-lrt` を、
  壁時計を使いたい場合は `-DUSE_GETTIMEOFDAY` を指定してビルドできます。

## ファイル

| ファイル | 説明 |
|---|---|
| `memspeed.c` | プログラム本体 |
| `Makefile` | ビルド定義（`all` / `clean` / `test`） |
| `LICENSE` | BSD 2-Clause License の全文 |
| `README.md` | このファイル |

## ライセンス

BSD 2-Clause License（SPDX 識別子: `BSD-2-Clause`）です。全文は `LICENSE` を、
また同じ文面は `memspeed.c` の先頭コメントにも記載しています。

```
Copyright (c) 1999-2026, Masato Minda <minmin@wide.ad.jp>
All rights reserved.
```

---

## English

A very small memory-bandwidth benchmark that does nothing but measure the
throughput of `memcpy(3)`.  The whole program is the single file `memspeed.c`,
and libc is its only dependency.

```
Written by Masato Minda <minmin@wide.ad.jp>
```

### Build

```sh
make			# builds memspeed
make test		# runs a few representative cases
make testnoopt		# runs the same cases without optimization, to compare
make clean
```

`CFLAGS` defaults to `-O2`, which the measurement can afford: the copy loop
also reads the last byte of both buffers through volatile-qualified pointers,
a side effect an optimizing compiler may neither drop nor move, so the
`memcpy()` calls being measured cannot be optimized away (see the
implementation notes below).  The level still shapes how a copy ends up being
carried out, so `make testnoopt` runs the same cases over a build with an empty
`CFLAGS` for comparison.  To check the source for warnings, build with
`make CFLAGS='-O2 -Wall -Wextra -pedantic'` -- a `CFLAGS` given on the command
line replaces the one in the Makefile, `-O2` included -- and it is expected to
produce no warnings.

### Usage

```
Usage: memspeed [-offset] size count
	-offset: shift the destination by offset bytes
	   size: allocate size (k/KB, m/MB, g/GB; a bare number is MB)
	  count: repeat count
```

- `size`: how much to copy in one round.  A number without a suffix means
  megabytes; `k`/`m`/`g` (upper case is accepted, and a trailing `b` as well)
  select KB, MB or GB.
- `count`: how many times that copy is repeated.
- `-offset`: shifts the destination N bytes away from its start, which is how
  the penalty of a misaligned copy can be measured.

```sh
$ ./memspeed 10 10
copy 10 MB at 10 times (total 100 MB + 0 KB) in 0.022838 sec  4379 MB/sec

$ ./memspeed 128k 1000
copy 128 KB at 1000 times (total 128 MB + 0 KB) in 0.005626 sec  22752 MB/sec

$ ./memspeed -8 4 100
copy 4 MB at 100 times (total 400 MB + 0 KB) in 0.062776 sec  6372 MB/sec
```

The exit status is 0 on success, and 1 for a bad argument, a failed allocation
or `-h`/`--help`.  Every diagnostic goes to stderr, so the report line printed
on stdout can be piped away on its own.

### Units

Everything is decimal (1 KB = 1000 bytes, 1 MB = 1000 KB), so the `MB/sec`
figure at the end of a report line is
`total bytes copied / elapsed seconds / 1,000,000`.  It is not MiB/sec.

### Implementation notes

- **The buffers are filled with a non-zero pattern (0xA5) before measuring.**
  Pages of a freshly malloc'ed buffer are commonly backed by the kernel's shared
  zero page, so an all-zero buffer would let part of the copy avoid real memory
  traffic and make the machine look faster than it is.
- **Elapsed time comes from `CLOCK_MONOTONIC` (nanosecond resolution) where it
  is available.**  A wall clock with microsecond resolution easily reports
  0 sec for a short run, and old revisions divided the total by exactly that
  zero.  When the elapsed time is 0 sec the speed is reported as `N/A`, and
  stderr suggests raising `size` or `count`; a run shorter than one millisecond
  gets a different notice instead.
- If the resolution feels too coarse, use a larger `size` or `count`.
- **Optimizing the program no longer optimizes the benchmark away.**  A loop
  that keeps writing the same bytes somewhere nobody reads is something `-O2`
  is entitled to delete as a whole, and an `cc -O2` build really did report
  0 sec and `N/A` in older revisions.  Each copy is now followed by a
  comparison of the last byte of the source with the last byte of the
  destination, both read through volatile-qualified pointers: a volatile read
  is a side effect that cannot be removed, and its value can only match the
  source if the copy actually ran, so the compiler has to keep the copies.
  Were they ever to disagree, `copy mismatch` goes to stderr, the loop stops
  there and the total is computed from the copies that did finish.
  This `-O2` build is the default now; `make testnoopt` measures the same cases
  without optimization, and the two have not come apart so far.
- An absurdly large `size` may get the process killed by the operating system
  instead of waiting for `malloc()` to return NULL and print `Can't alloc
  memory`.

### Portability notes

- C99 or later is required (`stdint.h` and `strtoull()` are used).
- The Makefile only delegates to `$(CC)`, so it builds as it is on macOS, Linux
  and Solaris.  Every mainstream compiler accepts the default `-O2`; drop it
  with `make CFLAGS=` if one ever mishandles the source.
- On older systems where `clock_gettime()` does not live in libc, build with
  `LDLIBS=-lrt`.  To force the traditional wall clock implementation instead,
  build with `-DUSE_GETTIMEOFDAY`.

### Files

| File | Description |
|---|---|
| `memspeed.c` | the program itself |
| `Makefile` | build rules (`all` / `clean` / `test`) |
| `LICENSE` | full text of the BSD 2-Clause License |
| `README.md` | this file |

### License

BSD 2-Clause License (SPDX identifier: `BSD-2-Clause`).  The full text is in
`LICENSE` and is repeated in the header comment of `memspeed.c`.

```
Copyright (c) 1999-2026, Masato Minda <minmin@wide.ad.jp>
All rights reserved.
```

[日本語に戻る / Back to the Japanese version](#memspeed)

