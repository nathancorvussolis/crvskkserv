
-------------------------------------------------------------------------------

   crvskkserv 2.1.1

-------------------------------------------------------------------------------

SKK辞書サーバです。IPv4、IPv6に対応しています。

適当な場所に置いて実行して下さい。

タスクトレイのアイコンから設定ダイアログを開き、ポートと辞書を設定して下さい。

辞書は、SKK辞書ファイル、SKK辞書サーバ、Google CGI API for Japanese Input に
対応しています。辞書の最大数は64個です。

改行LFでEUC-JIS-2004やUTF-8(BOMなし)等ASCIIと互換性のある文字コードのSKK辞書
ファイルが使用できます。

SKK辞書の文字コード変換は行っていないので、クライアント側のSKK辞書サーバ通信に
使用する文字コードとSKK辞書ファイルの文字コードを揃える必要があります。

Google CGI APIの「見出し語の検索除外条件」設定は、おそらく送りあり(と思われる)
見出し語を除外する正規表現がデフォルトとなっています。
Visual C++ 2013 のTR1正規表現で、文法はECMAScriptを使用しています。
http://msdn.microsoft.com/en-us/library/bb982727%28v=vs.120%29.aspx

Google CGI APIの設定でクライアントの文字コードをEUC-JIS-2004にしたとき、
UTF-8からEUC-JIS-2004に変換できない文字を含む候補は除外されます。

プロトコル

"0"
	切断
	応答なし

"1<見出し語> "
	辞書検索
	応答 : 検索結果 "1/<候補 1>/<候補 2>/ … /<候補 n>\n"
		または 検索結果なし "4\n"

"2"
	バージョン番号取得
	応答 : "crvskkserv/<major>.<minor>.<build> "
		(本家skkservでは、"<major>.<minor>.<revision> ")

"3"
	ホスト名取得
	応答 : "<hostname>/<ip address>:<port>/ "
		(本家skkservでは、"<hostname>:<ip address>: " または
			"<hostname>:<ip address 1>:<ip address 2>: … :<ip address n>: ")

"4<見出し語> "
	サーバーコンプリーション
	応答 : 検索結果なし "4\n"

その他
	応答なし

-------------------------------------------------------------------------------

   LICENSE

-------------------------------------------------------------------------------

crvskkserv

The MIT License

Copyright (C) 2012-2015 SASAKI Nobuyuki

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.

-------------------------------------------------------------------------------
