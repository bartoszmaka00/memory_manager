
//Autor: Bartosz Mąka
#include "heap.h"
#include <stdio.h>
#include <string.h>
#include "custom_unistd.h"
#define BLOK sizeof(struct memory_chunk_t)
#define FENCE 16*sizeof(char)
#define PAGE_SIZE 4096
struct memory_manager_t memory_manager;
void display(){
    struct memory_chunk_t *w=memory_manager.first_memory_chunk;
    if(!w)return ;
    int i=1;
    while(w!=NULL){
        printf("%d.adres: %p, adres prev: %p, adres next: %p,free: %d, rozmiar: %d  \n",i,(void*)w,(void*)w->prev,(void*)w->next,w->free,(int)w->size);
        w=w->next;
        i++;
    }
}
int heap_setup(void)
{
    memory_manager.memory_start=custom_sbrk(0);
    memory_manager.memory_end=NULL;
    memory_manager.first_memory_chunk=NULL;
    memory_manager.memory_size=0;
    if(memory_manager.memory_start==(void*)-1)return -1;
    return 0;

}
void heap_clean(void)
{
    if(memory_manager.memory_start!=NULL){
        custom_sbrk((-1)*custom_sbrk_get_reserved_memory());
        memory_manager.memory_start=NULL;
        memory_manager.memory_end=NULL;
        memory_manager.first_memory_chunk=NULL;
        memory_manager.memory_size=0;
    }
}
void* heap_malloc(size_t size)
{
    if(heap_validate()!=0 || size<=0)return NULL;//poprawa
    if(memory_manager.first_memory_chunk==NULL)
    {
        void*wsk=custom_sbrk(size+BLOK+2*FENCE);
        if(wsk==(void*)-1)return NULL;
        wsk=custom_sbrk(0);
        memory_manager.memory_end=wsk;
        memory_manager.memory_size=(char*)memory_manager.memory_end-(char*)memory_manager.memory_start;
        memory_manager.first_memory_chunk=memory_manager.memory_start;
        memory_manager.first_memory_chunk->next=NULL;
        memory_manager.first_memory_chunk->prev=NULL;
        memory_manager.first_memory_chunk->size=size;
        memory_manager.first_memory_chunk->free=0;
        memory_manager.first_memory_chunk->checksum=control_sum(memory_manager.first_memory_chunk);
        plotka(memory_manager.first_memory_chunk);
        return  (char *)memory_manager.first_memory_chunk+BLOK+FENCE;
    }
    else{
        struct memory_chunk_t *w=memory_manager.first_memory_chunk;
        if(!w)return NULL;
        if(w->free==1 && w->size-2*FENCE>=size){
            w->size=size;
            w->free=0;
            w->checksum=control_sum(w);
            plotka(w);
            return (char*)w+BLOK+FENCE;
        }
        while(w->next!=NULL){
            if(w->free==1 && w->size-2*FENCE>=size){
                w->size=size;
                w->free=0;
                w->checksum=control_sum(w);
                plotka(w);
                return (char*)w+BLOK+FENCE;
            }
            w=w->next;
        }
        if(w->free==1){
            w->size=size;
            w->free=0;
            w->checksum=control_sum(w);
            plotka(w);
            return (char*)w+BLOK+FENCE;
        }
        void*wsk=custom_sbrk(size+BLOK+2*FENCE);
        if(wsk==(void*)-1)return NULL;
        wsk=custom_sbrk(0);
        memory_manager.memory_end=wsk;
        memory_manager.memory_size=(char*)memory_manager.memory_end-(char*)memory_manager.memory_start;
        w->next=(struct memory_chunk_t*)((char*)w+BLOK+w->size+2*FENCE);
        w->next->prev=w;
        w->next->next=NULL;
        w->next->size=size;
        w->next->free=0;
        w->checksum=control_sum(w);
        w->next->checksum=control_sum(w->next);
        plotka(w->next);
        return (char*)w->next+BLOK+FENCE;
    }
}
void* heap_calloc(size_t number, size_t size)
{
    size_t rozmiar=number*size;
    void*ptr=heap_malloc(rozmiar);
    if(!ptr)return NULL;
    memset(ptr,0,rozmiar);
    return ptr;
}
void* heap_realloc(void* memblock, size_t count)
{

    if(!memblock && count>0)return heap_malloc(count);
    if(memblock && count==0){
        heap_free(memblock);
        return NULL;
    }
    if(count<=0 || get_pointer_type(memblock)!=pointer_valid || heap_validate()!=0)return NULL;
    struct memory_chunk_t *block=(struct memory_chunk_t *)((char*)memblock-BLOK-FENCE);
    if(block->size==count)return memblock;
    else if(count<block->size){
        if(block->next==NULL){
            custom_sbrk(-(block->size-count));
            void *wsk=custom_sbrk(0);
            memory_manager.memory_end=wsk;
            memory_manager.memory_size=(char*)memory_manager.memory_end-(char*)memory_manager.memory_start;
        }
        block->size=count;
        block->checksum=control_sum(block);
        plotka(block);
        return memblock;
    }
    //zwiekszanie jesli jest miejsce miedzy blokami
    if(block->next!=NULL && count>block->size){
        if((char*)block->next-(char*)block-BLOK-2*FENCE >= count){
            block->size=count;
            block->checksum=control_sum(block);
            plotka(block);
            return memblock;
        }
    }
    //zwiekszanie jesli w next free = 1 i brakuje miejsca w wskazanym bloku
    if(block->next!=NULL && block->next->free==1){
        if((char*)block->next-(char*)block-BLOK-2*FENCE+block->next->size >= count){
            size_t ile=count-(size_t)((char*)block->next-(char*)block-BLOK-2*FENCE);
            struct memory_chunk_t temp2;
            memcpy(&temp2,block->next,sizeof(struct memory_chunk_t));
            struct memory_chunk_t *k2=block->next->next;
            struct memory_chunk_t *temp=(struct memory_chunk_t*)((char*)block->next+ile);

            temp->next=temp2.next;
            temp->prev=temp2.prev;
            temp->size=temp2.size-ile;
            temp->free=temp2.free;
            temp->checksum=control_sum(temp);
            if(k2!=NULL){
                k2->prev=temp;
                k2->checksum=control_sum(k2);
            }
            block->next=temp;
            block->size=count;
            block->checksum=control_sum(block);
            plotka(block);
            return memblock;
        }
    }
    //zwiekszenie tego samego bloku jesli jest jeden/ostatni
    if((char*)block==(char*)memory_manager.first_memory_chunk && block->next==NULL && count>block->size){
        void*wsk=custom_sbrk(count-block->size);
        if(wsk==(void*)-1)return NULL;
        memory_manager.memory_end=wsk;
        block->size=count;
        block->checksum=control_sum(block);
        plotka(block);
        return memblock;
    }

    void *newblock=heap_malloc(count);
    if(!newblock)return NULL;
    memcpy(newblock,memblock,block->size);
    heap_free(memblock);
    return newblock;
}
void  heap_free(void* memblock)
{
    if(!memblock || heap_validate()!=0)return;
    struct memory_chunk_t*p=(struct memory_chunk_t *)((char*)memblock-BLOK-FENCE);
    struct memory_chunk_t*w=memory_manager.first_memory_chunk;
    while(w!=NULL)
    {
        if(w==p)break;
        w=w->next;
    }
    if(w!=p)return;
    p->free=1;
    heap_free_help(p);
}
void  heap_free_help(struct memory_chunk_t* p)
{
    if(p==NULL)return;
    if(p->free==0)return;
    if(!p->next)
    {
        if(p->prev){
            p->prev->next=NULL;
            p->prev->checksum=control_sum(p->prev);
        }
        char*wsk=custom_sbrk(0);
        int do_zwolnienia=(int)((char*)wsk-(char*)p);
        memory_manager.memory_size-=do_zwolnienia;
        custom_sbrk(-do_zwolnienia);
        memory_manager.memory_end=custom_sbrk(0);
        if(p==memory_manager.first_memory_chunk){
            memory_manager.first_memory_chunk=NULL;
            return;
        }
    }
    else
    {
        p->size=(char*)p->next-(char*)p-BLOK;
            if(p->next->free==1)
            {
                if(p->next->next){
                    p->size=(char*)p->next->next-(char*)p-BLOK;
                    p->next=p->next->next;
                    p->next->prev=p;
                    p->checksum=control_sum(p);
                    p->next->checksum=control_sum(p->next);
                }
                else{
                    char*wsk=custom_sbrk(0);
                    int do_zwolnienia=(int)((char*)wsk-(char*)p->next);
                    memory_manager.memory_size-=do_zwolnienia;
                    custom_sbrk(-do_zwolnienia);
                    memory_manager.memory_end=custom_sbrk(0);
                    p->size=(char*)p->next-(char*)p-BLOK;

                    p->next=NULL;
                    p->checksum=control_sum(p);
                }
            }
            p->checksum=control_sum(p);

    }
    heap_free_help(p->prev);
}
size_t   heap_get_largest_used_block_size(void)
{
    if(memory_manager.memory_start==NULL||memory_manager.first_memory_chunk==NULL || heap_validate()!=0)return 0;
    struct memory_chunk_t*p=memory_manager.first_memory_chunk;
    size_t ile=0;
    while(p!=NULL){
        if(p->size>ile && p->free==0)ile=p->size;
        p=p->next;
    }
    return ile;
}
enum pointer_type_t get_pointer_type(const void* const pointer)
{
    if(pointer==NULL)return pointer_null;
    if(heap_validate()!=0)return pointer_heap_corrupted;
    struct memory_chunk_t*p=memory_manager.first_memory_chunk;
    while(p!=NULL){
        char *temp=(char*)p;
        if(p->free==1){
            p=p->next;
            continue;
        }
        if((char*)pointer>=temp && (char*)pointer<temp+BLOK){
            return pointer_control_block;
        }
        else if(((char*)pointer>=temp+BLOK && (char*)pointer<temp+BLOK+FENCE) || ((char*)pointer>=temp+BLOK+FENCE+p->size && (char*)pointer<temp+BLOK+p->size+2*FENCE)){
            return pointer_inside_fences;
        }
        else if((char*)pointer==temp+BLOK+FENCE){
            return pointer_valid;
        }
        else if((char*)pointer>temp+BLOK+FENCE && (char*)pointer<temp+BLOK+p->size+FENCE){
            return pointer_inside_data_block;
        }
        p=p->next;
    }
    return pointer_unallocated;
}
int heap_validate(void)
{
    if(memory_manager.memory_start==NULL && memory_manager.first_memory_chunk==NULL)return 2;
    struct memory_chunk_t*go=(struct memory_chunk_t*)memory_manager.first_memory_chunk;
    while(go!=NULL){
        char*w=(char*)go+BLOK;
        int checksum=control_sum(go);
        if(checksum!=go->checksum)return 3;
        if(go->free==0)
        {
            for(int i=0;i<32;i++)
            {
                if(i<16 && *(w+i)!='#')return 1;
                if(i>=16 && *(w+i+go->size)!='#')return 1;
            }
        }
        go=go->next;
    }
    return 0;
}
void plotka(struct memory_chunk_t*p)
{
    if(!p)return;
    char* w=(char*)p+BLOK;
    for(int i=0;i<32;i++)
    {
        if(i<16)*(w+i)='#';
        if(i>=16)*(w+i+p->size)='#';
    }
}
int control_sum(const struct memory_chunk_t *p)
{

    struct memory_chunk_t temp;
    memcpy(&temp,p,sizeof(struct memory_chunk_t));
    temp.checksum=0;
    uint8_t * m=(uint8_t *)&temp;
    int sum=0;
    for(int i=0;i<(int)sizeof(struct memory_chunk_t);i++){
        sum+= *m++;
    }
    return sum;
}
void* heap_malloc_aligned(size_t count)
{
    if(heap_validate()!=0 || count<=0)return NULL;
    if(memory_manager.first_memory_chunk==NULL)
    {
        void*p=heap_malloc(PAGE_SIZE-(BLOK+2*FENCE+BLOK+FENCE));
        if(!p)return NULL;
        void*w=heap_malloc(count);
        if(!w)return NULL;
        heap_free(p);
        return  (char *)w;
    }
    else{
        struct memory_chunk_t *w=memory_manager.first_memory_chunk;
        char*temp;
        while(w!=NULL){
            temp=(char*)w+BLOK;
            if(w->free==1)
            {
                for(int i=0;i<(int)w->size;i++)
                {
                    if(((intptr_t)temp+BLOK+FENCE & (intptr_t)(PAGE_SIZE - 1))==0 && count/PAGE_SIZE<w->size/PAGE_SIZE && (long)((char*)w->next-(temp+BLOK+FENCE))>= (long)(count+FENCE) &&(int)((char*)temp-(char*)w)>(int)(BLOK+FENCE+BLOK)){
                        struct memory_chunk_t *m=(struct memory_chunk_t *)temp;
                        void *wsk=malloc_inside_aligned(m,w,count);
                        return wsk;
                    }
                    temp++;
                }
            }
            w=w->next;
        }
        int space=(int)(PAGE_SIZE - (memory_manager.memory_size%PAGE_SIZE)-FENCE);
        if(space>0 && space>(int)(BLOK+2*FENCE+BLOK+FENCE))
        {
            space=(int)(PAGE_SIZE - (memory_manager.memory_size % PAGE_SIZE));
        }
        else{
            space=(int)(PAGE_SIZE - (memory_manager.memory_size % PAGE_SIZE) + PAGE_SIZE);
        }
        void*p;
        p=heap_malloc_help(space-BLOK-2*FENCE-BLOK-FENCE);
        if(!p)return NULL;
        void*k=heap_malloc_help(count);
        if(!k)return NULL;
        heap_free(p);
        return k;
    }
}
void* heap_malloc_help(size_t size)
{
    if(heap_validate()!=0 || size<=0)return NULL;
        struct memory_chunk_t *w=memory_manager.first_memory_chunk;
        if(!w)return NULL;
        while(w->next!=NULL){
            w=w->next;
        }
        void*wsk=custom_sbrk(size+BLOK+2*FENCE);
        if(wsk==(void*)-1)return NULL;
        wsk=custom_sbrk(0);
        memory_manager.memory_end=wsk;
        memory_manager.memory_size=(char*)memory_manager.memory_end-(char*)memory_manager.memory_start;
        w->next=(struct memory_chunk_t*)((char*)w+BLOK+w->size+2*FENCE);
        w->next->prev=w;
        w->next->next=NULL;
        w->next->size=size;
        w->next->free=0;
        w->checksum=control_sum(w);
        w->next->checksum=control_sum(w->next);
        plotka(w->next);
        return (char*)w->next+BLOK+FENCE;
}
void* malloc_inside_aligned(struct memory_chunk_t *m,struct memory_chunk_t *w,size_t count)
{
    //w=wolny
    //m=struktura nowa na poczatku strony
    m->prev=w;
    m->next=w->next;
    if(m->next!=NULL)m->next->prev=m;
    w->next=m;
    w->size=((char*)m-(char*)w)-BLOK;
    m->size=count;
    m->free=0;
    m->checksum=control_sum(m);
    w->checksum=control_sum(w);
    if(m->next!=NULL)m->next->checksum=control_sum(m->next);
    plotka(m);

   return ((char*)m+BLOK+FENCE);
}
void* heap_calloc_aligned(size_t number, size_t size)
{
    size_t rozmiar=number*size;
    void*ptr=heap_malloc_aligned(rozmiar);
    if(!ptr)return NULL;
    memset(ptr,0,rozmiar);
    return ptr;
}
void* heap_realloc_aligned(void* memblock, size_t size)
{
    if(!memblock && size>0){
        void*wsk=heap_malloc_aligned(size);
        return wsk;
    }
    if(memblock && size==0){
        heap_free(memblock);
        return NULL;
    }
    if(size<=0 || get_pointer_type(memblock)!=pointer_valid || heap_validate()!=0)return NULL;
    struct memory_chunk_t *block=(struct memory_chunk_t *)((char*)memblock-BLOK-FENCE);
    if(block->size==size)return memblock;
    else if(size<block->size){
        if(block->next==NULL){
            custom_sbrk(-(block->size-size));
            void *wsk=custom_sbrk(0);
            memory_manager.memory_end=wsk;
            memory_manager.memory_size=(char*)memory_manager.memory_end-(char*)memory_manager.memory_start;
        }
        block->size=size;
        block->checksum=control_sum(block);
        plotka(block);
        return memblock;
    }
    //zwiekszanie jesli jest miejsce miedzy blokami
    if(block->next!=NULL && size>block->size){
        if((char*)block->next-(char*)block-BLOK-2*FENCE >= size){
            block->size=size;
            block->checksum=control_sum(block);
            plotka(block);
            return memblock;
        }
    }
    //zwiekszanie jesli w next free = 1 i brakuje miejsca w wskazanym bloku
    if(block->next!=NULL && block->next->free==1){
        if((char*)block->next-(char*)block-BLOK-2*FENCE+block->next->size >= size){
            size_t ile=size-(size_t)((char*)block->next-(char*)block-BLOK-2*FENCE);
            struct memory_chunk_t temp2;
            memcpy(&temp2,block->next,sizeof(struct memory_chunk_t));
            struct memory_chunk_t *k2=block->next->next;
            struct memory_chunk_t *temp=(struct memory_chunk_t*)((char*)block->next+ile);

            temp->next=temp2.next;
            temp->prev=temp2.prev;
            temp->size=temp2.size-ile;
            temp->free=temp2.free;
            temp->checksum=control_sum(temp);
            if(k2!=NULL){
                k2->prev=temp;
                k2->checksum=control_sum(k2);
            }
            block->next=temp;
            block->size=size;
            block->checksum=control_sum(block);
            plotka(block);
            return memblock;
        }
    }
    //zwiekszenie tego samego bloku jesli jest jeden/ostatni
    if(block->next==NULL && size>block->size){
        void*wsk=custom_sbrk(size-block->size);
        if(wsk==(void*)-1)return NULL;
        wsk=custom_sbrk(0);
        memory_manager.memory_end=wsk;
        memory_manager.memory_size=(char*)memory_manager.memory_end-(char*)memory_manager.memory_start;
        block->size=size;
        block->checksum=control_sum(block);
        plotka(block);
        return memblock;
    }
    //brak miejsca pomiedzy wiec wstawianie na koncu nowego
    int space=(int)(PAGE_SIZE - (memory_manager.memory_size%PAGE_SIZE)-FENCE);
    if(space>0 && space>(int)(BLOK+2*FENCE+BLOK+FENCE))
    {
        space=(int)(PAGE_SIZE - (memory_manager.memory_size % PAGE_SIZE));
    }
    else{
        space=(int)(PAGE_SIZE - (memory_manager.memory_size % PAGE_SIZE) + PAGE_SIZE);
    }
    void*p;
    p=heap_malloc_help(space-BLOK-2*FENCE-BLOK-FENCE);
    if(!p)return NULL;
    void*newblock=heap_malloc_help(size);
    if(!newblock)return NULL;
    memcpy(newblock,memblock,block->size);
    heap_free(memblock);
    heap_free(p);
    return newblock;
}






