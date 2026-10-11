# memspeed

`memcpy(3)` のスループットを測るだけの、ごく小さなメモリ帯域ベンチマークです。
本体は `memspeed.c` 1ファイル、依存は libc のみです。

```
Written by Masato Minda <minmin@wide.ad.jp>
```

## ビルド

```sh
make			# memspeed が生成される
make test		# 代表的なケースを数回実行
make clean
```

`CFLAGS` は意図的に空です。最適化を有効にするとコンパイラが `memcpy()` を
単なるメモリコピーより速い処理に置き換えてしまうことがあり、それはこの
プログラムが測りたい対象そのものだからです。警告チェックをしたいときは
`make CFLAGS='-Wall -Wextra -pedantic'` としてください（現状、警告は出ません）。

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
  標準エラーに「size か count を大きくするか、最適化なしでビルドする」案内を出します。
  1 ミリ秒未満しか計測できなかった場合は別の注意文を出します。
- 分解能が不足すると感じる場合は `size` または `count` を大きくしてください。
- **`-O1` 以上でビルドすると数値が当てになりません。** コンパイラは同じ
  バッファへの繰り返しコピーを検出してループごと削除することがあり、実際に
  `cc -O2` でビルドすると計測時間が 0 秒になり `N/A` と表示されます。
  デフォルトの `Makefile` は `CFLAGS` を空にしています。
- 極端に大きい `size` を指定すると、OS のオーバーコミットによりプロセスが
  強制終了されることがあります（`Can't alloc memory` を待たずに kill される
  ことがあります）。

## ポートビルドメモ

- C99 以降（`stdint.h` と `strtoull()` を使います）。
- macOS / Linux / Solaris で `$(CC)` に任せるだけの素朴な Makefile です。
- `clock_gettime()` が libc に含まれない古いシステムでは `LDLIBS=-lrt` を、
  壁時計を使いたい場合は `-DUSE_GETTIMEOFDAY` を指定してビルドできます。

## ファイル

| ファイル | 説明 |
|---|---|
| `memspeed.c` | プログラム本体 |
| `Makefile` | ビルド定義（`all` / `clean` / `test`） |
| `README.md` | このファイル |
