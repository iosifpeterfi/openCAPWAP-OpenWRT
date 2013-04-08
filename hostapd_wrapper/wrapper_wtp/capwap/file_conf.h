#define CONFIGFILE "hostapd_wtp.conf"
#define VARLENGTH 1024 

struct config_wtp {
	int wtp_port;
	char ip_wtp[50];
	char path_unix_socket[50];
}config_wtp;

int max_(int integer1,int integer2){
	if(integer1>integer2)return integer1;
	return integer2;
}

int countString(char string[]){
	int cnt=0;
	for(cnt=0;string[cnt]!=0;cnt++);
	return cnt;
}

int isEqualString(char String1[],char String2[]){
	int len1=countString(String1);
	int len2=countString(String2);
	int i;
	for( i=0;i<max_(len1,len2);++i ){
		if ( String1[i]==0 && String2[i]==0 )return 1;
		if ( String1[i]==0 || String2[i]==0 )return 0;
		if ( String1[i]!=String2[i] )return 0;
	}
	return 1;
}

int StartWith(char String1[],char String2[]){
	int len1=countString(String1);
	int len2=countString(String2);
	int i;
	
	for( i=0; i<len2;i++ ){

		if ( String1[i]!=String2[i] )return 0;
	}

	return 1;
}

void ReplaceString(char *String1,char *rep,char *String2){	
	if(isEqualString(String1,rep)){
		String1[0]=0;
		return;
	}
	if(String1[0]==0 || rep[0]==0)return;
	int i,j,k,b,l,m;
	char tmp[strlen(String1)+strlen(String2)+1];
	for(i=0;i<=strlen(String1)+strlen(String2)-1;i++)tmp[i]=88;
	tmp[i]=0;
	for(i=0;i<=strlen(String1)-1;i++){
		if(strlen(rep)>=1)b=1;
		for(j=0;j<=strlen(rep)-1;j++){
			if(String1[i+j]!=rep[j]){ 
				b=0; 
				break; 
			}
		}	
		if(b){
			for(k=0;k<i;k++)tmp[k]=String1[k];
			if(strlen(String2)>0){
				for(m=0;m<=strlen(String2)-1;++m,k++)tmp[k]=String2[m];
			}
			for(l=i+j;l<=strlen(String1)-1;l++,k++)tmp[k]=String1[l];
			tmp[k]=0;

			for(k=0;k<strlen(tmp);k++)String1[k]=tmp[k];
			if (String1[k-1]==10)String1[k-1]=0;
			else 	String1[k]=0;	
		}
	}
}

void ReadConfiguration(struct config_wtp *con_wtp){
	FILE *file;

    file=fopen(CONFIGFILE,"r");

	char ss[VARLENGTH];
	
	sprintf(con_wtp->ip_wtp,"");
	sprintf(con_wtp->path_unix_socket,"");
	con_wtp->wtp_port=0;

	while(1){
		if (fgets(ss,VARLENGTH,file)==NULL)break;
		if(ss[0]=='\r' || ss[0]=='\n' || ss[0]=='#')continue;

		if(StartWith(ss,"ip_daemon_wtp")){
			ReplaceString(ss, "ip_daemon_wtp", "");
			ReplaceString(ss, "=", "");
			ReplaceString(ss, "\n", "");
			ReplaceString(ss, " ", "");
			sprintf(con_wtp->ip_wtp, "%s", ss);
			
		}else if(StartWith(ss,"sock_pach_wtp")){
			ReplaceString(ss, "sock_pach_wtp", "");
			ReplaceString(ss, "=", "");
			ReplaceString(ss, "\n", "");
			ReplaceString(ss, " ", "");
			sprintf(con_wtp->path_unix_socket, "%s", ss);
			
		}else if(StartWith(ss,"port_daemon_wtp")){
			ReplaceString(ss, "port_daemon_wtp","");
			ReplaceString(ss, "\n", "");
			ReplaceString(ss, " ", "");
			ReplaceString(ss, "=", "");
			con_wtp->wtp_port = atoi(ss);
		}

	}
	fclose(file);
	return;
}


