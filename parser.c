#include "common.h"

/*
* �����ȫ����Ϣ
*/
//ȫ���ļ����
FILE* fp;

//ͷ�ṹ��
struct BinlogEventHeader format_description_event_header;

//��ṹmap
map<string,string> mp; //���� ��Ӧ���ֶ� ��
map<long,string> table_mp; //���id ��Ӧ�ı�
map<long,vector<int> > type_mp; //��ÿ���ֶζ�Ӧ������
////////

int main()
{
    fp = fopen("mysql-bin.000003", "rb");
    if(fp == NULL)
    {
        printf("File not exist!\n");
        return -1;
    }
    int magic_number;
    //�ж�magic num �Ƿ�����
    fread(&magic_number, 4, 1, fp);

    printf("%d - %s\n", magic_number, (char*)(&magic_number));
    if(magic_number != MAGIC_NUMBER)
    {
        printf("File is not BinLog!\n");
        fclose(fp);
        return -1;
    }

    fseek(fp, 0L, SEEK_END);		//��λ�ļ�ָ�뵽β��
	long file_len = ftell(fp);		//����ļ�����
	fseek(fp, 4L, SEEK_SET);		//�Ѿ���ȡ��4�ֽڵ�ͷ

    //�� �� ����
    while(!feof(fp))
    {
        fread(&format_description_event_header, HEADER_LEN, 1, fp);
        // ��ӡ������ͷ��Ϣ
        //print_Header();

        // ���Event�Ľ���λ��
        int next_position = format_description_event_header.next_position;

        /* �������� ����EVENT��TYPE*/
        switch (format_description_event_header.type_code)
        {
        case FORMAT_DESCRIPTION_EVENT:
            //printf("Type : FORMAT_DESCRIPTION_EVENT\n");
            process_format();
            break;
        case QUERY_EVENT:
            //printf("Type : QUERY_EVENT\n");
            process_query();
            break;
        case TABLE_MAP_EVENT:
            //printf("Type : TABLE_MAP_EVENT\n");
            process_table_map();
            break;
        case WRITE_ROWS_EVENT:
            //printf("Type : WRITE_ROWS_EVENT\n");
            process_write();
            break;
        case UPDATE_ROWS_EVENT:
            //printf("Type : UPDATE_ROWS_EVENT\n");
            process_update();
            break;
        case DELETE_ROWS_EVENT:
            //printf("Type : DELETE_ROWS_EVENT\n");
            process_delete();
            break;
        //����commit
        case XID_EVENT:
            //printf("Type : XID_EVENT\n");
            process_xid();
            break;
        }
        //�ƶ�ָ�뵽��һ��λ��
        printf("!!!!!!! %d\n",next_position);
        if(next_position == file_len) break;
        fseek(fp,next_position,SEEK_SET);
    }
    return 0;
}


void print_Header()
{
    printf("BinlogEventHeader\n{\n");
    printf("    timestamp: %d\n", format_description_event_header.timestamp);
    printf("    type_code: %d\n", format_description_event_header.type_code);
    printf("    server_id: %d\n", format_description_event_header.server_id);
    printf("    event_length: %d\n", format_description_event_header.event_length);
    printf("    next_position: %d\n", format_description_event_header.next_position);
    printf("    flags[]: %d\n}\n", format_description_event_header.flags);
}

/*
* format_desc Event ����
*/
void process_format()
{
    short binlog_version;
    fread(&binlog_version, 2, 1, fp);
    //printf("binlog_version: %d\n", binlog_version);

    char server_version[51];
    fread(server_version, 50, 1, fp);
    server_version[50] = '\0';
    //printf("server_version: %s\n", server_version);

    int create_timestamp;
    fread(&create_timestamp, 4, 1, fp);
    //printf("create_timestamp: %d\n", create_timestamp);

    char header_length;
    fread(&header_length, 1, 1, fp);
    //printf("header_length: %d\n", header_length);

    int type_count = format_description_event_header.event_length - 76;
    unsigned char post_header_length[type_count];
    fread(post_header_length, 1, type_count, fp);
    /*
    for(int i = 0; i < type_count; i++)
    {
        printf("  - type %d: %d\n", i + 1, post_header_length[i]);
    }
    */
}

/*
* ��ȡ�����Ӧ���ֶ�
*/
void fuck(string database_name,string sql)
{
    //printf("%s\n %s",database_name,sql);
    string table_name="";
    table_name+=database_name+'.' ;
    /*���ж� �Ƿ��Ǵ�����*/
    int idx = sql.find("CREATE");
    if(idx == string::npos)
    {
        idx = sql.find("create");
        if(idx == string::npos)
        {
            return ;
        }
    }
    idx = sql.find("TABLE");
    if(idx==string::npos)
    {
        idx = sql.find("table");
        if(idx==string::npos) return ;
    }
    //cout<<sql<<endl;
    idx+=5;
    for(;sql[idx]==' ' || sql[idx]=='\n' || sql[idx]=='\r';idx++) ;
    while(sql[idx]!=' ')
    {
        table_name+=sql[idx++];
    }
    /*�� ���ݿ���.���� ����ʽ��¼�ñ�*/
    cout<<"Debug:"<<table_name<<endl;

    /*������ṹ*/
    string table_content="(";
    int len = sql.size();
    int end_pos;
    for(int i=len-1;i>=0;i--)
    {
        if(sql[i]==')') { end_pos = i; break; }
    }
    //1.�ҵ���һ��(
    idx = sql.find("(");
    string tmp ="`";
    while(true)
    {
        if((sql[idx]>='a' && sql[idx]<='z')||
            (sql[idx]>='A' && sql[idx]<='Z'))
        break;
        idx++;
    }
    for(;sql[idx]!=' ';idx++)
        tmp+=sql[idx];
    table_content+=tmp+"`";

    //2.�ҵ�,
    while(true)
    {
        idx = sql.find(",",idx);
        if(idx==string::npos || idx > end_pos)
        {
            //������
            table_content+=")";
            break;
        }
        else
        {
            tmp="";
            //ȥ���ո�
            while(true)
            {
                if((sql[idx]>='a' && sql[idx]<='z')||
                    (sql[idx]>='A' && sql[idx]<='Z'))
                break;
                idx++;
            }
            for(;sql[idx]!=' ';idx++)
                tmp+=sql[idx];
            //�ų��������ȵĿ���
            if(tmp=="PRIMARY" || tmp=="primary" || tmp=="UNIQUE" || tmp=="unique")
            {
                //������
                table_content+=')';
                break;
            }
            else
            {
                table_content+=",`";
                table_content+=tmp+"`" ;
            }
        }
        //cout<<table_content<<endl;
        //system("pause");
    }
    cout<<"Debug:" <<table_content<<endl;
    mp[table_name] = table_content;
}

/*
* QUERY_EVENT ����
*/
void process_query()
{
    /*fixed part*/
    int thread_id;
    fread(&thread_id,4,1,fp);
    //printf("thread_id: %d\n",thread_id);

    int process_time;
    fread(&process_time,4,1,fp);
    //printf("process_time: %d\n",process_time);

    /*���ݿ� ����*/
    char len;
    fread(&len,1,1,fp);
    //printf("database len: %d\n",len);

    short errorcode;
    fread(&errorcode,2,1,fp);
    //printf("error code: %d\n",errorcode);

    short status_vars_block_len ;
    fread(&status_vars_block_len,2,1,fp);
    //printf("status_vars_block_len : %d\n",status_vars_block_len);

    //��ԽһЩû�õ���Ϣ
    char buffer[1024];
    fread(buffer,status_vars_block_len,1,fp);

    char database_name[512];
    fread(database_name,len+1,1,fp);
    database_name[len]='\0';
    printf("use '%s';\n",database_name);

    /*variable parts*/
    char sql[1024];
    memset(sql,0,sizeof(sql));
    //sql len = event_length - status_vars_block_len - �̶�ͷ���� - ���ݿ������ - fixed length - У��� 4�ֽ�
    int sql_len = format_description_event_header.event_length - status_vars_block_len - 19 -13 -len-1 -4;
    fread(sql,sql_len,1,fp);
    sql[sql_len] = '\0';
    //Ϊ����ȡInsert/upsate/delete���ֶ� ��Ҫ��������Ӧ��ĸ����ֶ���

    fuck((string)database_name,(string)sql);

    printf("%s\n",sql);
    printf("***************************\n");
    //system("pause");
}

/*
* TABLE_MAP ����
*/
void process_table_map()
{
    /*fixed part*/
    long table_id;
    fread(&table_id,6,1,fp);
    //printf("table_id : %d\n",table_id);

    //��ԽһЩû�õ���Ϣ
    char buffer[3];
    fread(buffer,2,1,fp);

    /*variable part*/
    //���ݿ������� and���ݿ���
    char database_name_len ;
    fread(&database_name_len,1,1,fp);
    printf("database len: %d\n",database_name_len);

    char database_name[512];
    fread(&database_name,database_name_len+1,1,fp);
    database_name[database_name_len]='\0';
    printf("database name: %s\n",database_name);

    //�������� and ����
    char table_name_len ;
    fread(&table_name_len,1,1,fp);
    printf("database len: %d\n",table_name_len);

    char table_name[512];
    fread(&table_name,table_name_len+1,1,fp);
    table_name[table_name_len]='\0';
    printf("table name: %s\n",table_name);

    string tmp ="";
    tmp+= database_name;
    tmp+=".";
    tmp+=table_name;
    table_mp[table_id]= tmp;

    cout<<"Debug: "<<mp[tmp]<<endl;
    /*�ֶ� type*/
    char cnt_type;
    fread(&cnt_type,1,1,fp);
    cout<<"Debug :"<<(int)cnt_type<<endl;


    vector<int> vt;
    for(int i=0;i<(int)cnt_type;i++)
    {
        char tmp_type;
        fread(&tmp_type,1,1,fp);
        cout<<(int)tmp_type<<endl;
        switch(tmp_type)
        {
        case MYSQL_TYPE_TINY:
            vt.push_back(C_CHAR);
            break;
        case MYSQL_TYPE_SHORT:
            vt.push_back(C_SHORT);
            break;
        case MYSQL_TYPE_LONG:
            vt.push_back(C_INT);
            break;
        case MYSQL_TYPE_FLOAT:
            vt.push_back(C_FLOAT);
            break;
        case MYSQL_TYPE_DOUBLE:
            vt.push_back(C_DOUBLE);
            break;
        case MYSQL_TYPE_LONGLONG:
            vt.push_back(C_LONG_LONG);
            break;
        case MYSQL_TYPE_TIMESTAMP2:
        case MYSQL_TYPE_DATETIME2:
        case MYSQL_TYPE_TIMESTAMP:
        case MYSQL_TYPE_DATETIME:
            vt.push_back(C_TIME);
            break;
        default :
            vt.push_back(C_CHARS);
            break;
        }
    }
    for(int i=0;i<vt.size();i++)
        cout<<"Debug :"<<vt[i]<<" ";
    cout<<endl;
    printf("*******************\n");
    //����id ���ֶ���������ӳ��
    type_mp[table_id] = vt;
    //system("pause");
}

/*
* WRITE_ROWS_EVENT/DELETE_ROWS_EVENT/UPDATE_ROWS_EVENT ����
* ����sql����־����һ�� ��ͬ�ľ�����ǰ��� insert \delete\update
* ��Ϊinsert ����Ϊ���ӣ�ʹ�ù̶���ʽ��ʾ sql
* insert into + ���� + (�ֶ�) + values + ( ���� )
*/
void process_write()
{
    /*fixed part*/
    long table_id;
    fread(&table_id,6,1,fp);
    //printf("table_id : %d\n",table_id);

    //��ԽһЩû�õ���Ϣ
    char buf[5];
    fread(buf,4,1,fp);
    /*variable part*/
    //�ֶ�����
    char cnt_type;
    fread(&cnt_type,1,1,fp);
    printf("Debug : %d\n",(int)cnt_type);

    //����FF Դ������Ϊ m_cols;	/* Bitmap denoting columns available */
    char t;
    fread(&t,1,1,fp);

    cout<<"INSERT INTO "<<table_mp[table_id]<<" "<<mp[table_mp[table_id]]
        <<" VALUES (";

    //����ÿ���ֶ��Ƿ�Ϊ��
    //cnt_type���ֶ� ��Ҫ  cnt_type+7 /8 �ֽ� =n
    // s[n] �洢ÿ���ֽ� С�˴洢 Ҫ����������
    //0 ��ʾ����null | 1=null
    int tmp;
    int n = (cnt_type + 7) / 8;
    char s[n+1]; s[n]='\0';
    fread(&s,n,1,fp);
    int m=n-1; //������ֽڿ�ʼ��ǰ��
    //�ܹ�cnt_type���ֶ�
    vector<int> vt = type_mp[table_id];

    for(int i=0;i<cnt_type;i++)
    {
        //vector<int> vt = type_mp[table_id];
        tmp = ( s[ m-(i/8) ]>> (i%8) )&1 ;
        if(tmp == 1)
        {
            //null
            //cout<<"NULL"<<endl;
            cout<<"'NULL'";
            if(i!=cnt_type-1) cout<<",";
        }
        else
        {
            //����null ���ж�����

            //cout<<vt[i]<<endl;
            switch(vt[i])
            {
            case C_CHAR:
                char type;
                fread(&type,1,1,fp);
                cout<<"'"<<type<<"'";
                break;
            case C_SHORT:
                short type1;
                fread(&type1,2,1,fp);
                cout<<"'"<<type1<<"'";
                break;
            case C_INT:
                int type2;
                fread(&type2,4,1,fp);
                cout<<"'"<<type2<<"'";
                break;
            case C_FLOAT:
                float type3;
                fread(&type3,4,1,fp);
                cout<<"'"<<type3<<"'";
                break;
            case C_DOUBLE:
                double type4;
                fread(&type4,8,1,fp);
                cout<<"'"<<type4<<"'";
                break;
            case C_LONG_LONG:
                long long type5;
                fread(&type5,8,1,fp);
                cout<<"'"<<type5<<"'";
                break;
            default :
                char type6[1024];
                short len ;
                fread(&len,2,1,fp);
                cout<<"len: "<<len<<endl;
                fread(&type6,len,1,fp);
                type6[len]='\0';
                cout<<"'"<<type6<<"'";
                break;
            }
            if(i!=cnt_type-1) cout<<",";
        }
    }
    cout<<")"<<endl;

    //system("pause");
}
/*
* update
* ����һ���鷳����Ϊ�������ļ��ǰ���˳�����ģ�һ���ļ�д���˺�ͻ�д����һ���ļ�
* �����������һ���ļ�����û��������Ϣ ��ʱupdate��䲻����ȷ����
* ����ʹ�� update + ���� + where + ������ǰ�� + set + (���ĺ�) ������ʾ
*/
//TO DO: ʱ�����ʽ��Ϊ 99 A0 65 50 41 B5 ��֪����ô���� -> 2018-07-18 21:01:01
void process_update()
{
    /*fixed part*/
    long table_id;
    fread(&table_id,6,1,fp);
    printf("table_id : %d\n",table_id);

    //��ԽһЩû�õ���Ϣ
    char buf[5];
    fread(&buf,4,1,fp);

    /*variable part*/
    //�ֶ�����
    char cnt_type;
    fread(&cnt_type,1,1,fp);
    printf("Debug : %d\n",(int)cnt_type);

    //����FF Դ������Ϊ m_cols;	/* Bitmap denoting columns available */
    char t[3];
    fread(&t,2,1,fp);

    cout<<"UPDATE "<<table_mp[table_id]<<" WHERE ";

    //����ÿ���ֶ��Ƿ�Ϊ��
    //cnt_type���ֶ� ��Ҫ  cnt_type+7 /8 �ֽ� =n
    // s[n] �洢ÿ���ֽ� С�˴洢 Ҫ����������
    //0 ��ʾ����null | 1=null
    int tmp;
    int n = (cnt_type + 7) / 8;

    //��Ҫ�������Σ�����ǰ �� ���ĺ������
    for(int j=0;j<2;j++)
    {
        char s[n+1]; s[n]='\0';
        fread(&s,n,1,fp);
        int m=n-1; //������ֽڿ�ʼ��ǰ��
        //�ܹ�cnt_type���ֶ�
        vector<int> vt = type_mp[table_id];
        //for(int i=0;i<vt.size();i++)
         //   cout<<vt[i]<<endl;
        //system("pause");
        cout<<"(";

        for(int i=0;i<cnt_type;i++)
        {
            //vector<int> vt = type_mp[table_id];
            tmp = ( s[ m-(i/8) ]>> (i%8) )&1 ;
            if(tmp == 1)
            {
                //null
                //cout<<"NULL"<<endl;
                cout<<"'NULL'";
                if(i!=cnt_type-1) cout<<",";
            }
            else
            {
                //����null ���ж�����

                //cout<<vt[i]<<endl;
                switch(vt[i])
                {
                case C_CHAR:
                    char type;
                    fread(&type,1,1,fp);
                    cout<<"'"<<type<<"'";
                    break;
                case C_SHORT:
                    short type1;
                    fread(&type1,2,1,fp);
                    cout<<"'"<<type1<<"'";
                    break;
                case C_INT:
                    int type2;
                    fread(&type2,4,1,fp);
                    cout<<"'"<<type2<<"'";
                    break;
                case C_FLOAT:
                    float type3;
                    fread(&type3,4,1,fp);
                    cout<<"'"<<type3<<"'";
                    break;
                case C_DOUBLE:
                    double type4;
                    fread(&type4,8,1,fp);
                    cout<<"'"<<type4<<"'";
                    break;
                case C_LONG_LONG:
                    long long type5;
                    fread(&type5,8,1,fp);
                    cout<<"'"<<type5<<"'";
                    break;
                case C_TIME:
                    char type_time[6];
                    fread(&type_time,5,1,fp);
                    cout<<"'TIME'";
                    break;
                default :
                    char type6[1024];
                    short len ;
                    fread(&len,2,1,fp);
                    //cout<<"len: "<<len<<endl;
                    fread(&type6,len,1,fp);
                    type6[len]='\0';
                    cout<<"'"<<type6<<"'";
                    break;
                }
                if(i!=cnt_type-1) cout<<",";
            }
        }
        cout<<")"<<endl;

        //system("pause");
    }
}


/*
* delete
* ����������������һ��
* delete from + ���� + (�ֶ�) + set + ( ���� )
*/
//TO DO: ʱ�����ʽ��Ϊ 99 A0 65 50 41 B5 ��֪����ô���� -> 2018-07-18 21:01:01
void process_delete()
{
    /*fixed part*/
    long table_id;
    fread(&table_id,6,1,fp);
    //printf("table_id : %d\n",table_id);

    //��ԽһЩû�õ���Ϣ
    char buf[5];
    fread(buf,4,1,fp);
    /*variable part*/
    //�ֶ�����
    char cnt_type;
    fread(&cnt_type,1,1,fp);
    printf("Debug : %d\n",(int)cnt_type);

    //����FF Դ������Ϊ m_cols;	/* Bitmap denoting columns available */
    char t;
    fread(&t,1,1,fp);

    cout<<"DELETE FROM "<<table_mp[table_id]<<" WHERE "
        <<mp[table_mp[table_id]]<<" SET (";

    //����ÿ���ֶ��Ƿ�Ϊ��
    //cnt_type���ֶ� ��Ҫ  cnt_type+7 /8 �ֽ� =n
    // s[n] �洢ÿ���ֽ� С�˴洢 Ҫ����������
    //0 ��ʾ����null | 1=null
    int tmp;
    int n = (cnt_type + 7) / 8;
    char s[n+1]; s[n]='\0';
    fread(&s,n,1,fp);
    int m=n-1; //������ֽڿ�ʼ��ǰ��
    //�ܹ�cnt_type���ֶ�
    vector<int> vt = type_mp[table_id];

    for(int i=0;i<cnt_type;i++)
    {
        //vector<int> vt = type_mp[table_id];
        tmp = ( s[ m-(i/8) ]>> (i%8) )&1 ;
        if(tmp == 1)
        {
            //null
            //cout<<"NULL"<<endl;
            cout<<"'NULL'";
            if(i!=cnt_type-1) cout<<",";
        }
        else
        {
            //����null ���ж�����

            //cout<<vt[i]<<endl;
            switch(vt[i])
            {
            case C_CHAR:
                char type;
                fread(&type,1,1,fp);
                cout<<"'"<<type<<"'";
                break;
            case C_SHORT:
                short type1;
                fread(&type1,2,1,fp);
                cout<<"'"<<type1<<"'";
                break;
            case C_INT:
                int type2;
                fread(&type2,4,1,fp);
                cout<<"'"<<type2<<"'";
                break;
            case C_FLOAT:
                float type3;
                fread(&type3,4,1,fp);
                cout<<"'"<<type3<<"'";
                break;
            case C_DOUBLE:
                double type4;
                fread(&type4,8,1,fp);
                cout<<"'"<<type4<<"'";
                break;
            case C_LONG_LONG:
                long long type5;
                fread(&type5,8,1,fp);
                cout<<"'"<<type5<<"'";
                break;
            default :
                char type6[1024];
                short len ;
                fread(&len,2,1,fp);
                //cout<<"len: "<<len<<endl;
                fread(&type6,len,1,fp);
                type6[len]='\0';
                cout<<"'"<<type6<<"'";
                break;
            }
            if(i!=cnt_type-1) cout<<",";
        }
    }
    cout<<")"<<endl;

    //system("pause");
}

/*
* XID_EVENT ����
*/
void process_xid()
{
    printf("COMMIT;\n");
}
